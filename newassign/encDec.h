/***************************** PHYSICAL LAYER ******************************/
char* text2bin(char*);
bool parity(unsigned int);
bool verifyParity(char*);
char* bin2text(char*);


/***************************** DATA-LINK LAYER ******************************/
char* frame(char*);
char* deframe(char*);


/***************************** APPLICATION LAYER *******************************/
char* encodeData(char*);
char* decodeData(char*);
