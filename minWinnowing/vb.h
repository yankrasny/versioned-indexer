#define VBYTE_ENCODE(_v, _n)	\
{\
	unsigned _num;				\
	unsigned char _barray[5];	\
	unsigned _i, _started = 0;	\
 	_num = _n;					\
	for (_i = 0; _i < 5; _i++)	\
	{							\
		_barray[_i] = ((_num%128)<<1);	\
		_num = _num/128;		\
	}							\
	for (_i = 4; _i > 0; _i--)	\
	{							\
		if ((_barray[_i] != 0) || (_started == 1))	\
		{						\
			_started = 1;		\
			*_v = _barray[_i]|0x1;	\
			_v++;				\
		}						\
	}							\
	*_v = _barray[0]|0x0;		\
	_v++;						\
}
                                                                     
                                                                     
                                                                     
                                             

#define VBYTE_DECODE1(_v, _n)	\
{\
	_n = ((*_v>>1));						\
	if ((*_v&0x1) != 0)		\
        {							\
          _v++;				\
	  _n = (_n<<7) + ((*_v>>1));	\
	  if ((*_v&0x1)!= 0)		\
          {						\
            _v++;				\
	    _n = (_n<<7) + ((*_v>>1));	\
	    if ((*_v&0x1) != 0)		\
            {						\
              _v++;				\
	      _n = (_n<<7) + ((*_v>>1));	\
	    }\
	  }\
	}\
        _v++;				\
}

/* another version. Note that now the hi bit (128) in the byte is the flag */
#define VBYTE_DECODE2(_v, _n)	\
{\
	_n = ((*_v)&127);		\
	if ((*_v) >= 128)		\
        {			        \
          _v++;				\
	  _n = (_n<<7) + ((*_v)&127);	\
	  if ((*_v) >= 128)		\
          {				\
            _v++;			\
	    _n = (_n<<7) + ((*_v)&127);	\
	    if ((*_v) >= 128)		\
            {				\
              _v++;			   \
	      _n = (_n<<7) + ((*_v)&127);  \
	    }                           \
	  }                             \
	}                               \
        _v++;				\
}					

/* 3rd version. Note that here also the hi bit (128) in the byte is the flag */
/* you need to define somewhere unsigned ints x0, x1, x2, x3; */
#define VBYTE_DECODE3(_v, _n)	\
{                               \
   if ((_x0 = (*_v)) < 128)	\
   {			        \
     _n = _x0;                     \
     _v++;		           \
   }                               \
   else                            \
     if ((_x1 = (*(_v+1)) < 128)   \
     {                             \
       _n = ((_x0&127)<<7) + _x1;  \
       _v += 2;			   \
     }                             \
     else                          \
       if ((_x2 = (*(_v+2)) < 128) \
       {                                                \
         _n = ((_x0&127)<<14) + ((_x1&127)<<7) + _x2;	\
         _v += 3;                  			\
       }                                                \
       else                                             \
       {                                                                \
         _n = ((_x0&127)<<21) + ((_x1&127)<<14) + ((_x2&127)<<7) + _x3; \
         _v += 4;                       		                \
       }                                                                \
}										

#define VBYTE_DECODE(_v, _n)	\
{\
	_n = ((*_v>>1));						\
	if ((*_v&0x1) != 0)		\
        {							\
          _v++;				\
	  _n = (_n<<7) + ((*_v>>1));	\
	  if ((*_v&0x1)!= 0)		\
          {						\
            _v++;				\
	    _n = (_n<<7) + ((*_v>>1));	\
	    if ((*_v&0x1) != 0)		\
            {						\
              _v++;				\
	      _n = (_n<<7) + ((*_v>>1));	\
	    }\
	  }\
	}\
        _v++;				\
}
