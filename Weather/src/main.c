#include <stdio.h>
#include "../includes/cities.h"
#include "../includes/utils.h"


int main(){
  Cities cities;
  printf("Welcome!\n");
  printf("Please select a city from the list below to get a weather report\n");
  while (1) {

    cities_init(&cities);

    if (utils_break_loop() != 0) {
      break;
    }
  }

  return 0;
}
