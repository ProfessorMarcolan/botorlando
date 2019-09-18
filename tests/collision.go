package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

func main() {
	m := make(map[uint]string)
	r := bufio.NewScanner(os.Stdin)
	for r.Scan() {
		t := r.Text()
		ts := strings.Split(t, " ")
		i, err := strconv.Atoi(ts[1])
		if err != nil {
			panic(err)
		}
		if key, ok := m[uint(i)]; ok {
			fmt.Printf("hash %d colide com %q e %q\n", i, ts[0], key)
			continue
		}
		m[uint(i)] = ts[0]
	}
}
