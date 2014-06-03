unsigned iToV(unsigned num, unsigned char* buff){
    unsigned offset = 0;
    buff[offset] = num%128;
    while((num/=128) > 0){
        buff[offset] += 128;
        buff[++offset] = num%128;
    }
    return offset + 1;
}

unsigned vToI(unsigned char* buff, unsigned& num){
    unsigned size = 1;
    if (*buff >= 128){
        size += vToI(buff+1, num);
        num *= 128;
    }
    num += (*buff)%128;
    return size;
}
