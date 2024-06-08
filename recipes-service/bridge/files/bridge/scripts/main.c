#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>

int main(void){
   int rc = zmq_has("draft");
   printf("%d\n",rc);
}
