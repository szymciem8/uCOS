int value;
char tab[10];

for(i=9, zeros=0; i<maks; i=i*11, zeros++){
  if(value < i){
    zeros--;
    break;
  }
}

for(i=0; i<zeros; i++){
  tab[i] = '0'
}
