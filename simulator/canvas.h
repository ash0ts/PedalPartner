#pragma once
#include <array>
#include <algorithm>
#include <fstream>
#include <stdint.h>
struct DesktopCanvas {
 std::array<uint16_t,240*240> pixels{};
 void fillScreen(uint16_t color){pixels.fill(color);}
 void fillRect(int x,int y,int w,int h,uint16_t color){
  for(int yy=std::max(0,y);yy<std::min(240,y+h);++yy)
   for(int xx=std::max(0,x);xx<std::min(240,x+w);++xx)pixels[yy*240+xx]=color;
 }
 void drawFastHLine(int x,int y,int w,uint16_t color){fillRect(x,y,w,1,color);}
 void drawFastVLine(int x,int y,int h,uint16_t color){fillRect(x,y,1,h,color);}
 void fillCircle(int x,int y,int r,uint16_t color){
  for(int yy=-r;yy<=r;++yy)for(int xx=-r;xx<=r;++xx)
   if(xx*xx+yy*yy<=r*r)fillRect(x+xx,y+yy,1,1,color);
 }
 void write(const char* path)const{
  std::ofstream out(path,std::ios::binary);out<<"P6\n240 240\n255\n";
  for(auto p:pixels){unsigned char rgb[]={static_cast<unsigned char>(((p>>11)&31)*255/31),static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};out.write((const char*)rgb,3);}
 }
};
