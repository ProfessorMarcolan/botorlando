package main

import (
	"math/rand"
	"os"
)

var b = []string{
	"badge-info",
	"bits",
	"badges",
	"color",
	"display-name",
	"emote-only",
	"emotes",
	"flags",
	"id",
	"mod",
	"room-id",
	"subscriber",
	"tmi-sent-ts",
	"turbo",
	"user-id",
	"user-type",
	"login",
	"message",
	"msg-id",
	"target-msg-id",
	"tag-name",
	"number-of-viewers",
	"followers-only",
	"msg-param-cumulative-months",
	"msg-param-displayName",
	"msg-param-login",
	"msg-param-promo-gift-total",
	"msg-param-promo-name",
	"msg-param-months",
	"msg-param-recipient-display-name",
	"msg-param-recipient-id",
	"msg-param-recipient-user-name",
	"msg-param-sender-login",
	"hosting_channel",
	"msg-param-sender-name",
	"msg-param-should-share-streak",
	"msg-param-streak-months",
	"msg-param-sub-plan",
	"msg-param-sub-plan-name",
	"msg-param-viewerCount",
	"msg-param-ritual-name",
	"msg-param-threshold",
	"r9k",
	"subs-only",
	"ban-duration",
	"slow",
}

func main() {

	m := map[[32]byte]struct{}{}
Loop:
	for {

		strl := rand.Intn(32)
		if strl <= 0 {
			continue
		}
		if len(m) > 59 {
			m = nil
			m = make(map[[32]byte]struct{})
		}
		var buf [32]byte
		for i := 0; i < strl; i++ {
			char := rand.Intn(126)
			for ; !isValid(char); char = rand.Intn(126) {
			}
			buf[i] = byte(char)
		}
		if _, ok := m[buf]; ok {
			continue Loop
		}
		m[buf] = struct{}{}
		os.Stdout.Write(buf[:])
		os.Stdout.WriteString("\n")
	}
}

func isValid(c int) bool {
	switch {
	case c >= '0' && c <= '9':
		return true
	case c >= 'A' && c <= 'Z':
		return true
	case c >= 'a' && c <= 'z':
		return true
	case c == '-' || c == '_':
		return true
	default:
		return false
	}
}
