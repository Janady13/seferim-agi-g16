package main

import (
	"bufio"
	"crypto/sha1"
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"time"
)

type IndexEntry struct {
	Path      string    `json:"path"`
	Size      int64     `json:"size"`
	ModTime   time.Time `json:"mod_time"`
	Hash      string    `json:"sha1"`
	Ext       string    `json:"ext"`
	Summary   string    `json:"summary,omitempty"`
	Language  string    `json:"language,omitempty"`
	FamilyTag string    `json:"family_tag,omitempty"` // placeholder to wire into G16 later
}

var allowed = map[string]string{
	".md": "text", ".txt": "text", ".json": "json", ".yml": "yaml", ".yaml": "yaml",
	".c": "c", ".h": "c", ".hpp": "cpp", ".hh": "cpp", ".hxx": "cpp", ".cpp": "cpp", ".cc": "cpp",
	".rs": "rust", ".go": "go", ".js": "js", ".ts": "ts", ".java": "java", ".cs": "csharp",
	".swift": "swift", ".kt": "kotlin", ".m": "objc", ".mm": "objc++",
	".png": "image", ".jpg": "image", ".jpeg": "image",
}

func hashFile(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := sha1.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

func shortSummary(path string) string {
	f, err := os.Open(path)
	if err != nil {
		return ""
	}
	defer f.Close()
	r := bufio.NewReader(f)
	var lines []string
	for i := 0; i < 3; i++ {
		l, err := r.ReadString('\n')
		if err != nil && err != io.EOF {
			break
		}
		lines = append(lines, strings.TrimSpace(l))
		if err == io.EOF {
			break
		}
	}
	return strings.Join(lines, " | ")
}

func main() {
	out := flag.String("out", "data/memory_index.jsonl", "output JSONL file")
	root := flag.String("root", os.Getenv("HOME"), "root to scan (default: $HOME)")
	skip := flag.String("skip", "/Library,/System,/Applications,/proc,/dev,/Volumes,/node_modules,/.git,/.cargo,/.cache", "comma-separated paths to skip")
	flag.Parse()

	outDir := filepath.Dir(*out)
	if err := os.MkdirAll(outDir, 0o755); err != nil {
		fmt.Fprintf(os.Stderr, "mkdir: %v\n", err)
		os.Exit(1)
	}
	fo, err := os.Create(*out)
	if err != nil {
		fmt.Fprintf(os.Stderr, "open out: %v\n", err)
		os.Exit(1)
	}
	defer fo.Close()
	w := bufio.NewWriter(fo)
	defer w.Flush()

	skips := strings.Split(*skip, ",")
	shouldSkip := func(p string) bool {
		for _, s := range skips {
			if strings.Contains(p, s) {
				return true
			}
		}
		return false
	}

	count := 0
	err = filepath.WalkDir(*root, func(path string, d os.DirEntry, err error) error {
		if err != nil {
			return nil // skip unreadable path
		}
		if d.IsDir() {
			if shouldSkip(path) {
				return filepath.SkipDir
			}
			return nil
		}
		ext := strings.ToLower(filepath.Ext(path))
		lang, ok := allowed[ext]
		if !ok {
			return nil
		}
		st, err := os.Stat(path)
		if err != nil {
			return nil
		}
		if st.Size() > 10*1024*1024 {
			return nil // skip very large files
		}
		h, err := hashFile(path)
		if err != nil {
			return nil
		}
		entry := IndexEntry{
			Path:     path,
			Size:     st.Size(),
			ModTime:  st.ModTime(),
			Hash:     h,
			Ext:      ext,
			Language: lang,
		}
		if lang == "text" || lang == "json" || lang == "yaml" || lang == "cpp" || lang == "go" || lang == "rust" || lang == "js" || lang == "ts" || lang == "java" || lang == "csharp" || lang == "swift" {
			entry.Summary = shortSummary(path)
		}
		b, _ := json.Marshal(entry)
		w.Write(b)
		w.WriteByte('\n')
		count++
		if count%2000 == 0 {
			w.Flush()
		}
		return nil
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "walk error: %v\n", err)
	}
	fmt.Fprintf(os.Stderr, "indexed %d files into %s\n", count, *out)
}
