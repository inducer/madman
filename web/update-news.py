#! /usr/bin/python
import feedparser
import codecs

feed = feedparser.parse("http://news.tiker.net/taxonomy/term/1/0/feed")

output = codecs.open("news-items.html", "w", "utf-8")
for item in feed.entries[1:4]:
  output.write("<div class=\"NewsHead\"><a href=\"%s\">%s</a></div>\n" % 
               (item.link, item.title))
  output.write("<div class=\"NewsBody\">%s</div>\n" % 
               item.summary)
