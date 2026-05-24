package main

import (
	"bufio"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"strings"
	"time"
)

type outputLine struct {
	Path    string `json:"path"`
	Type    string `json:"type"`
	Content string `json:"content"`
	Family  int    `json:"family"`
}

type wikiResp struct {
	Query struct {
		Pages []struct {
			Title    string `json:"title"`
			Extract  string `json:"extract"`
			Pageid   int    `json:"pageid"`
			Missing  any    `json:"missing,omitempty"`
			Invalid  any    `json:"invalid,omitempty"`
			InvalidR any    `json:"invalidreason,omitempty"`
		} `json:"pages"`
	} `json:"query"`
}

func fetchWiki(topic string) (string, error) {
	endpoint := "https://en.wikipedia.org/w/api.php"
	q := url.Values{}
	q.Set("action", "query")
	q.Set("prop", "extracts")
	q.Set("explaintext", "1")
	q.Set("format", "json")
	q.Set("formatversion", "2")
	q.Set("titles", topic)
	req, _ := http.NewRequest("GET", endpoint+"?"+q.Encode(), nil)
	req.Header.Set("User-Agent", "SEFERIM-Indexer/1.0 (+https://example.local)")

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return "", fmt.Errorf("http %d", resp.StatusCode)
	}
	var wr wikiResp
	if err := json.NewDecoder(resp.Body).Decode(&wr); err != nil {
		return "", err
	}
	if len(wr.Query.Pages) == 0 {
		return "", fmt.Errorf("no pages in response")
	}
	return wr.Query.Pages[0].Extract, nil
}

func main() {
	if len(os.Args) < 3 {
		fmt.Fprintf(os.Stderr, "usage: %s topics.csv out.jsonl\n", os.Args[0])
		os.Exit(2)
	}
	topicsFile := os.Args[1]
	outFile := os.Args[2]

	tf, err := os.Open(topicsFile)
	if err != nil {
		log.Fatal(err)
	}
	defer tf.Close()
	r := csv.NewReader(tf)
	r.FieldsPerRecord = -1

	out, err := os.Create(outFile)
	if err != nil {
		log.Fatal(err)
	}
	defer out.Close()
	w := bufio.NewWriter(out)
	defer w.Flush()

	for {
		rec, err := r.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatalf("csv: %v", err)
		}
        if len(rec) == 0 {
            continue
        }
		fam := 11
		topic := ""
		if len(rec) >= 2 {
			fmt.Sscanf(strings.TrimSpace(rec[0]), "%d", &fam)
			topic = strings.TrimSpace(rec[1])
		} else {
			topic = strings.TrimSpace(rec[0])
		}
		if topic == "" {
			continue
		}
		text, err := fetchWiki(topic)
		if err != nil {
			log.Printf("warn: %s: %v", topic, err)
			time.Sleep(200 * time.Millisecond)
			continue
		}
		line := outputLine{
			Path:    fmt.Sprintf("wiki/%s.txt", strings.ReplaceAll(topic, " ", "_")),
			Type:    "txt",
			Content: text,
			Family:  fam,
		}
		b, _ := json.Marshal(line)
		fmt.Fprintln(w, string(b))
		time.Sleep(200 * time.Millisecond)
	}

	log.Printf("done -> %s", outFile)
}

