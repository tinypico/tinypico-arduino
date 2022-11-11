char* string2char(String command)
{
    char *p = const_cast<char*>(command.c_str());
    return p;
}

void println_Center( Adafruit_ST7789 &d, String heading, int centerX, int centerY )
{
  if (heading.length() > 0)
  {
    int x = 0;
    int y = 0;
    int16_t  x1, y1;
    uint16_t ww, hh;

    char *p = const_cast<char*>(heading.c_str());
  
    d.getTextBounds( p, x, y, &x1, &y1, &ww, &hh );
    d.setCursor( centerX - ww / 2 + 2, centerY - hh / 2);
    d.println( heading );
  }
}
