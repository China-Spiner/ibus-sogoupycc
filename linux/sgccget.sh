#!/bin/bash

# Sogou Pinyin Cloud fetch
# Author: Jun WU <quark@lihdd.net>
# Licence: GPLv2

py=`echo $@ | sed "s# #'#g;s#[^a-z']##g;s#''#'#g"`
res=`wget -qO - --timeout 5 -t 3 "http://web.pinyin.sogou.com/web_ime/get_ajax/${py:-kongbai}.key"`

if [ $? -eq 0 ]; then
{
js << !
$res;
function urldecode(utftext){var string="",i=c=c1=c2=0;
while(i<utftext.length){c=utftext.charCodeAt(i);if(c<128){string+=String.fromCharCode(c);i++;}
else if((c>191)&&(c<224)){c2=utftext.charCodeAt(i+1);string+=String.fromCharCode(((c&31)<<6)|(c2&63));
i+=2;}else{c2=utftext.charCodeAt(i+1);c3=utftext.charCodeAt(i+2);
string+=String.fromCharCode(((c&15)<<12)|((c2&63)<<6)|(c3&63));i+=3;}}return string;}
print(urldecode(unescape(ime_query_res)));
!
} | while read i; do
		if echo $i | fgrep '：' 1>/dev/null 2>/dev/null; then
			echo -n $i | sed 's/：.*//';
			exit;
		fi
	done
else
	echo -n "网络错误(${py})"
fi

# for low network speed test:
# sleep $((RANDOM%15))
