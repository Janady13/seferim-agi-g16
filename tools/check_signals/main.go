package main

import (
	"encoding/json"
	"fmt"
	"os"
)

type S struct {
	DxNorm    *float64 `json:"dx_norm"`
	EdError   *float64 `json:"ed_error"`
	Utility   *float64 `json:"utility"`
	Stability *float64 `json:"stability"`
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "usage: check_signals <signals.json>\n")
		os.Exit(2)
	}
	b, err := os.ReadFile(os.Args[1])
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	var s S
	if err := json.Unmarshal(b, &s); err != nil {
		fmt.Fprintf(os.Stderr, "invalid JSON: %v\n", err)
		os.Exit(3)
	}
	ok := s.DxNorm != nil && s.EdError != nil && s.Utility != nil && s.Stability != nil
	if !ok {
		fmt.Fprintf(os.Stderr, "missing required keys\n")
		os.Exit(4)
	}
	fmt.Println("ok")
}

