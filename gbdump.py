# -*- coding:utf-8 -*-

import serial
import struct
import re
import time

logo = '\xce\xedff\xcc\r\x00\x0b\x03s\x00\x83\x00\x0c\x00\r\x00\x08\x11\x1f\x88\x89\x00\x0e\xdc\xccn\xe6\xdd\xdd\xd9\x99\xbb\xbbgcn\x0e\xec\xcc\xdd\xdc\x99\x9f\xbb\xb93'

bank_type = {
    "\x00":"ROM Only",
    "\x01":"ROM + MBC1",
    "\x02":"ROM + MBC1 + RAM",
    "\x03":"ROM + MBC1 + RAM + BATTERY",
    "\x05":"ROM + MBC2",
    "\x06":"ROM + MBC2 + BATTERY",
    "\x08":"ROM + RAM",
    "\x09":"ROM + RAM + BATTERY",
    "\x0F":"ROM + MBC3 + RTC + BATTERY",
    "\x10":"ROM + MBC3 + RTC + RAM + BATTERY",
    "\x11":"ROM + MBC3",
    "\x12":"ROM + MBC3 + RAM",
    "\x13":"ROM + MBC3 + RAM + BATTERY",
    "\x15":"ROM + MBC4",
    "\x16":"ROM + MBC4 + RAM",
    "\x17":"ROM + MBC4 + RAM + BATTERY",
    "\x19":"ROM + MBC5",
    "\x1A":"ROM + MBC5 + RAM",
    "\x1B":"ROM + MBC5 + RAM + BATTERY",
    "\x1C":"ROM + MBC5 + RUMBLE",
    "\x1D":"ROM + MBC5 + RUMBLE + RAM",
    "\x1E":"ROM + MBC5 + RUMBLE + RAM + BATTERY",
    #"\x1F":"Pocket Camera",
    "\x20":"ROM + MBC6 + ???",
    "\x22":"ROM + MBC7 + BATTERY",
    "\xFC":"Pocket Camera",
    "\xFD":"Bandai TAMA5",
    "\xFE":"Hudson HuC3",
    "\xFF":"Hudson HuC1(ROM + HuC1 + RAM + BATTERY)"
}

rom_sizes = {
    "\x00":"32KB",
    "\x01":"64KB",
    "\x02":"128KB",
    "\x03":"256KB",
    "\x04":"512KB",
    "\x05":"1MB",
    "\x06":"2MB",
    "\x07":"4MB",
    "\x08":"8MB"
}

ram_sizes = {
    "\x00":"Nothing or 512Bytes(MBC2)",
    "\x01":"2KB",
    "\x02":"8KB",
    "\x03":"32KB",
    "\x04":"128KB",
    "\x05":"64KB"
}

def readall(ser, mask=0b00000000):
    data = ""
    size = 0
    while True:
        val = ser.read()
        if(val == ""):
            break
        val = struct.unpack('B', val)[0] | mask
        data += struct.pack('B', val)
        #data += val
        size += 1
        if size % (1024*16) == 0:
            print("%dKB"%(size/1024))
    return data

def main():
    s = serial.Serial("COM4", 57600, timeout=1)
    s.write("0");
    head = bytes(readall(s))
    print("NINTENDO logo check: %s" % str(logo==head[0x0104:0x0133]))
    print("TITLE: %s" % head[0x0134:0x0142])
    print("ROM TYPE: %s" % bank_type[head[0x147]])
    print("ROM SIZE: %s" % rom_sizes[head[0x148]])
    print("RAM SIZE: %s" % ram_sizes[head[0x149]])

    select = ""
    while (not select in ["1", "2", "3"]):
        print("")
        print("DUMP ROM: 1")
        print("DUMP RAM: 2")
        print("WRITE TO RAM: 3")
        select = raw_input("? ")
    
    filetype = ""
    if(select == "1"):
        s.write("1")
        filetype = "gb"
    elif(select == "2"):
        s.write("2")
        filetype = "sav"
    elif(select == "3"):
        s.write("3")
        filetype = "sav"
    
    title = str(head[0x0134:0x0142].replace("\x00",""))
    title = re.sub(r'[\\|/|:|?|"|<|>|\|]', '_', title)
    filename = "%s.%s"%(title, filetype)

    if(select in ["1", "2"]):
        #MBC2でRAMを読むときは,上4bitを破棄する必要がある.
        if(select=="2" and (head[0x147]=="\x05" or head[0x147]=="\x06")):
            data = bytes(readall(s, 0b11110000))
        else:
            data = bytes(readall(s))
        f = open(filename, "w+b")
        f.write(data)
        f.close()
    
    elif(select == "3"):
        f = open(filename, "r")
        savedata = f.read()
        f.close()
        cnt = 1
        while True:
            #256Bごとの定期連絡
            if(cnt % 256 == 0):
                resp = readall(s)
                if("ACK" in resp):
                    time.sleep(0.01)
                    continue
            #終わったとき
            if(len(savedata) == cnt):
                break;
            #8KiBごとの出力
            if(cnt % (1024*8) == 0):
                print("%dKB"%(cnt/1024))

            #送信
            #data = struct.unpack('b', savedata[cnt-1])[0]
            data = savedata[cnt-1]
            s.write(data)

            cnt += 1

if(__name__=='__main__'):
    main()