#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "/home/gmazzini/local.c"

size_t output(void *ptr,size_t size,size_t nmemb,void *userdata){
    return size * nmemb;
}

int main(void) {
  CURL *curl;
  CURLcode res;
  long temp;
  FILE *fp;
  char buf[200];
  fp=fopen("/sys/class/thermal/thermal_zone0/temp","r");
  if(fp==NULL)return 0;
  fscanf(fp,"%ld",&temp);
  fclose(fp);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl=curl_easy_init();
  if(!curl) return 0;
  curl_easy_setopt(curl,CURLOPT_URL,URL);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,0L);
  curl_easy_setopt(curl,CURLOPT_TIMEOUT,5L);
  sprintf(buf,"key=%s&temp=%ld",MYKEY,temp/100);
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,buf);
  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,output);
  res=curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return 0;
}

