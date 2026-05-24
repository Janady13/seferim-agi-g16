package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"strings"
)

type Entry struct {
	Path     string `json:"path"`
	Size     int64  `json:"size"`
	Ext      string `json:"ext"`
	Language string `json:"language"`
}

type Signals struct {
	DxNorm    float64 `json:"dx_norm"`
	EdError   float64 `json:"ed_error"`
	Utility   float64 `json:"utility"`
	Stability float64 `json:"stability"`
	Files     int     `json:"files"`
}

func clamp01(x float64) float64 {
	if x < 0 {
		return 0
	}
	if x > 1 {
		return 1
	}
	return x
}

func main() {
	if len(os.Args) < 3 {
		fmt.Fprintf(os.Stderr, "usage: signals_from_index <index.jsonl> <out.json>\n")
		os.Exit(2)
	}
	in := os.Args[1]
	out := os.Args[2]

	f, err := os.Open(in)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer f.Close()

	typeCounts := map[string]int{}
	langs := map[string]int{}
	files := 0
	var sizeSum float64
	sc := bufio.NewScanner(f)
	for sc.Scan() {
		var e Entry
		if err := json.Unmarshal(sc.Bytes(), &e); err != nil {
			continue
		}
		files++
		sizeSum += float64(e.Size)
		ext := strings.ToLower(e.Ext)
		typeCounts[ext]++
		if e.Language != "" {
			langs[e.Language]++
		}
	}
	// normalized entropy of extensions
	K := float64(len(typeCounts))
	H := 0.0
	for _, c := range typeCounts {
		p := float64(c) / float64(files)
		H += -p * math.Log2(p)
	}
	Hnorm := 0.0
	if K > 1 {
		Hnorm = H / math.Log2(K)
	}
	// coverage of languages present over a canonical set
	canon := []string{"cpp", "go", "rust", "js", "ts", "java", "csharp", "swift", "kotlin"}
	covered := 0
	for _, k := range canon {
		if _, ok := langs[k]; ok {
			covered++
		}
	}
	coverage := float64(covered) / float64(len(canon))

	// signals
	dx := clamp01(0.5*Hnorm + 0.5*clamp01(sizeSum/1e7)) // entropy + size
	ed := clamp01(1.0 - coverage)                      // gap from full coverage

	// utility: prioritize if SEFERIM/AGI appears in paths
	util := 0.2
	if strings.Contains(strings.ToLower(in), "seferim") || strings.Contains(strings.ToLower(in), "agi") {
		util = 0.7
	}
	// stability higher when fewer file types (less chaos)
	stab := clamp01(1.0 - 0.5*Hnorm)

	s := Signals{DxNorm: dx, EdError: ed, Utility: util, Stability: stab, Files: files}
	b, _ := json.MarshalIndent(s, "", "  ")
	if err := os.MkdirAll(filepath.Dir(out), 0o755); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	if err := os.WriteFile(out, b, 0o644); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	fmt.Fprintf(os.Stderr, "signals -> %s (files=%d)\n", out, files)
}

