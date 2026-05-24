package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
)

type Signals struct {
	DxNorm    float64 `json:"dx_norm"`
	EdError   float64 `json:"ed_error"`
	Utility   float64 `json:"utility"`
	Stability float64 `json:"stability"`
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
		fmt.Fprintf(os.Stderr, "usage: merge_indices <signals1.json> <signals2.json> [...]\n")
		os.Exit(2)
	}
	sum := Signals{}
	for i := 1; i < len(os.Args); i++ {
		f, err := os.Open(os.Args[i])
		if err != nil {
			continue
		}
		var s Signals
		if err := json.NewDecoder(bufio.NewReader(f)).Decode(&s); err == nil {
			sum.DxNorm += s.DxNorm
			sum.EdError += s.EdError
			sum.Utility += s.Utility
			sum.Stability += s.Stability
		}
		f.Close()
	}
	n := float64(len(os.Args) - 1)
	if n > 0 {
		sum.DxNorm = clamp01(sum.DxNorm / n)
		sum.EdError = clamp01(sum.EdError / n)
		sum.Utility = clamp01(sum.Utility / n)
		sum.Stability = clamp01(sum.Stability / n)
	}
	b, _ := json.MarshalIndent(sum, "", "  ")
	os.Stdout.Write(b)
}

