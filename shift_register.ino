
int latchPin = 2;
int dataPin = 3;
int clockPin = 4;

int wr = 18;
int rd = 19;
int mreq = 20;

void setup() {
  Serial.begin(57600);
  //シフトレジスタ用
  for(int i=2; i<5; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  //データ受信用
  for(int i=5; i<13; i++)
    pinMode(i, INPUT);
  //WR, RD, CS_SRAM
  for(int i=18; i<21; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  //初期化
  digitalWrite(rd, HIGH);
  digitalWrite(mreq, HIGH);
  digitalWrite(wr, HIGH);
}

//シフトレジスタでアドレスを指定する.
void setaddress(unsigned int addr)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (addr>>8));
  shiftOut(dataPin, clockPin, MSBFIRST, (addr&0xFF));
  digitalWrite(latchPin, HIGH);
}

//値を一つだけ読み込んで返す
byte readdata(unsigned int addr)
{
    digitalWrite(rd, LOW);
    digitalWrite(mreq, LOW);
    setaddress(addr);
    delayMicroseconds(200);
    byte val = 0;
    for(int i=12; i>4; i--)
    {
      if(digitalRead(i) == HIGH)
      {
        bitWrite(val, i-5, HIGH);
      }
    }
    digitalWrite(rd, HIGH);
    digitalWrite(mreq, HIGH);
    return val;
}


//範囲を指定して一気に読み込む.そして送信する.
void readdata_range(unsigned int addr_start,unsigned int addr_end)
{
    for(unsigned int addr=addr_start; addr<=addr_end; addr++)
    {
      byte val = readdata(addr);
      Serial.write(val);
    }
}

//指定したアドレスにデータを書き込む
void writedata(unsigned int addr, unsigned int value)
{
  for(int i=5; i<13; i++)
    pinMode(i, OUTPUT);

  setaddress(addr);
  
  for(int i=0; i<8; i++)
  {
    if(value&(1<<i))
    {
      digitalWrite(i+5, HIGH);
    }
    else
    {
      digitalWrite(i+5, LOW);
    }
  }
  
  digitalWrite(wr, LOW);
  delay(50);
  digitalWrite(wr, HIGH);
  delay(50);
  
  for(int i=5; i<13; i++)
    pinMode(i, INPUT);
}

//romをdumpする.
void dumprom()
{
  int carttype = readdata(0x0147);
  int romsize = readdata(0x0148);

  //バンク数の計算
  //0x0148が0の時32KBで,そこから2倍づつ上がってゆく
  int banksize = (0b100000<<romsize) / 16;

  //MBC1
  if(carttype==0x01 || carttype==0x02 || carttype==0x03)
  {
    // ROM読み込みなので16/8モードで動作させる(起動時からそれ)
    unsigned int start_addr = 0x0000;
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x6000, 0 );
      writedata(0x4000, (i>>5)&0b11 );
      writedata(0x2000, i&0b11111);
      if(i>1)start_addr=0x4000;
      readdata_range(start_addr, 0x7FFF);
    }
  }
  //MBC2
  else if(carttype==0x05 || carttype==0x06)
  {
    unsigned int start_addr = 0x0000;
    for(int i=1; i<banksize; i++)
    {
      writedata(0x2100, i);
      if(i>1)start_addr=0x4000;
      readdata_range(start_addr, 0x7FFF);
    }
  }
  //MBC3
  else if(carttype==0x0F || carttype==0x10 || carttype==0x11 || carttype==0x12 || carttype==0x13)
  {    
    unsigned int start_addr = 0x0000;
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x2000, i&0b1111111);
      if(i>1)start_addr=0x4000;
      readdata_range(start_addr, 0x7FFF);
    }
  }
  //MBC5 or Pocket camera
  else if(carttype==0x19 || carttype==0x1A || carttype==0x1B || carttype==0x1C || carttype==0x1D || carttype==0x1E || carttype==0xFC)
  {
    unsigned int start_addr = 0x0000;
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x3000, (i>>8)&0b1 );
      writedata(0x2000, i&0b11111111);
      if(i>1)start_addr=0x4000;
      readdata_range(start_addr, 0x7FFF);
    }
  }
  else
    Serial.println("unknown type");
}

//ramをdumpする
void dumpram()
{
  int carttype = readdata(0x0147);
  int ramsize = readdata(0x0149);

  //書き込み保護 きやすめか?
  writedata(0x0000, 0xA);

  //バンク数の計算 RAMバンク1つにつき8KB
  int banksize = 0;
  if(ramsize == 0x00) banksize = 0;
  //else if (ramsize == 0x01) banksize = 2 / 8;
  else if (ramsize == 0x02) banksize = 8 / 8;
  else if (ramsize == 0x03) banksize = 32 / 8;
  else if (ramsize == 0x04) banksize = 128 / 8;
  else if (ramsize == 0x05) banksize = 64 / 8;

  if(banksize==0 && !(carttype==0x05 || carttype==0x06))
  {
    Serial.println("no ram exists.");
    return;
  }

  //MBC1
  if(carttype==0x01 || carttype==0x02 || carttype==0x03)
  {
    // 4/32モードで動作させる
    writedata(0x6000, 1 );
    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i&0b11 );
      readdata_range(0xA000, 0xBFFF);
    }
  }
  //MBC2
  else if(carttype==0x05 || carttype==0x06)
  {
     readdata_range(0xA000, 0xA1FF);
  }
  //MBC3
  else if(carttype==0x0F || carttype==0x10 || carttype==0x11 || carttype==0x12 || carttype==0x13)
  {
    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i&0b11 );
      readdata_range(0xA000, 0xBFFF);
    }
  }
  //MBC5 or Pocket camera
  else if(carttype==0x19 || carttype==0x1A || carttype==0x1B || carttype==0x1C || carttype==0x1D || carttype==0x1E || carttype==0xFC)
  {
    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i&0b11111111 );
      readdata_range(0xA000, 0xBFFF);
    }
  }
  else
    Serial.println("unknown type");
  writedata(0x0000, 0x0);
}

void write_ram()
{
    int cnt = 1;
    int data = 0;
    while(1)
    {
      if(Serial.available()>0)
      {
        data = Serial.read();
        Serial.print(data);
        cnt++;
        if(cnt%256 == 0)
        {
          Serial.println("ACK");
        }
      }
    }
}

void loop() {
  if(Serial.available()>0)
  {
    int data = Serial.read();
    // ヘッダ部を読み込む
    if(data == '0')
      readdata_range(0x0000, 0x0150);
    else if(data == '1')
      dumprom();
    else if(data == '2')
      dumpram();
    else if(data == '3')
      write_ram();
    else
      Serial.println("I could not read.");

    while(Serial.available()>0)
      Serial.read();
  }
}
