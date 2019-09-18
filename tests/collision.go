package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
	"sync"
)

func main() {
	r := bufio.NewScanner(os.Stdin)
	m := make(map[uint]string)
	colissions := []string{}
	var mu sync.Mutex

	for r.Scan() {
		t := r.Text()
		ts := strings.Split(t, " ")
		if len(ts) < 2 {
			break
		}
		i, err := strconv.Atoi(ts[1])
		if err != nil {
			panic(err)
		}
		if key, ok := m[uint(i)]; ok {
			mu.Lock()
			colissions = append(colissions, fmt.Sprintf("hash %d colide com %q e %q\n", i, ts[0], key))
			mu.Unlock()
			continue
		}
		mu.Lock()
		m[uint(i)] = ts[0]
		mu.Unlock()
	}
	if err := r.Err(); err != nil {
		panic(err)
	}
	mu.Lock()
	for _, v := range colissions {
		fmt.Print(v)
	}
	mu.Unlock()
	os.Exit(0)
}
