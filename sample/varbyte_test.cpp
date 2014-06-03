#include "../util/varbyte.h"
#include <iostream>

using namespace std;

int main(){

    unsigned i = 140;
    unsigned k = 1231311237;
    
    unsigned i_prime = 0;
    unsigned k_prime = 0;

    unsigned char buffer[20];

    unsigned offset = iToV(i, buffer);
    cout << "offset after writing " << i << ": " << offset << endl;
    offset += iToV(k, buffer+offset);
    cout << "offset after writing " << k << ": " << offset << endl;

    unsigned num_read = vToI(buffer, i_prime);
    cout << "read " << i_prime << ": num_read: " << num_read << endl;
    num_read = vToI(buffer+num_read, k_prime);
    cout << "read " << k_prime << ": num_read: " << num_read << endl;
    
    return 0;
}
