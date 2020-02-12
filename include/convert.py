o = open("video_font_big.h", "wt")
expLine = 0
with open("video_font.h") as f:
    for line in f:
        for i in line.split():
            if i.startswith('0x'):
                n = int(i[0:-1], 16)
		bn = bin(n)
		bitsNum = 0
		expandedByte = ''
		idx = len(bn)-1
		while bn[idx] != 'b':
         	    expandedByte += bn[idx] + bn[idx]
		    idx -=1
		    bitsNum +=1
		while bitsNum < 8:
		    expandedByte += '00'
		    bitsNum += 1

		bs = ''.join(reversed(expandedByte))
#		o.write(hex(int(bs[0:8], 2)) + ', ')
#                o.write(hex(int(bs[0:8], 2)) + ', ')
#                o.write(hex(int(bs[8:], 2)) + ', ')
#                o.write(hex(int(bs[8:], 2)) + ', ')
#                o.write(hex(int(bs[0:8], 2)) + ', ')
                o.write(hex(n) + ', ')
                o.write(hex(n) + ', ')

#		expLine += 1
#		if expLine == 16:
#		    while expLine > 0:
#			o.write(hex(255) + ', ')
#			expLine -= 1

#		o.write(hex(n) + ', ')
		
	    else:
		o.write(i.replace('16', '32').replace('8', '12') + ' ')
#                o.write(i.replace('16', '32').replace('8', '8') + ' ')
	o.write("\n")
