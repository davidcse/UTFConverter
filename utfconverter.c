#include "utfconverter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>


int main(int argc, char **argv)
{
    /*Declare variables */
    int opt=0, return_code = EXIT_FAILURE;
    char *input_path = NULL;
    char *output_path = NULL;
    int vargs =0;/* The number of V's that occurr */
    FILE* standardout = stdout; /* open output channel */
    char *encodeOutput; /*optional encode flag */

    /* Test */

    /* Parse short options */
    while((opt = getopt(argc, argv, "vhe:")) != -1) {
        switch(opt) {
            case 'h':  /* The help menu was selected */
                       USAGE(argv[0]);
                       exit(EXIT_SUCCESS);
                       break;
            case 'v':  if(vargs <3) vargs++;
                       break;
            case 'e':  encodeOutput = (optarg); /*grab the encode format */
                       break;
            case '?':  /* Let this case fall down to default handled during bad option.*/  
            
             default:  USAGE(argv[0]); /* A bad option was provided. */
                       exit(EXIT_FAILURE);   
        }
    }

    /* Get position arguments */
    if(optind < argc && ((argc - optind) == 2)) {
        input_path = argv[optind++];
        output_path = argv[optind++];
    } else {
        if((argc - optind) <= 0) {
            fprintf(standardout, "Missing INPUT_FILE and OUTPUT_FILE.\n");
        } else if((argc - optind) == 1) {
            fprintf(standardout, "Missing OUTPUT_FILE.\n");
        } else {
            fprintf(standardout, "Too many arguments provided.\n");
        }
        USAGE(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Make sure all the arguments were provided */
    if(input_path != NULL || output_path != NULL) {       
        switch(validate_args(input_path, output_path)) {
          case VALID_ARGS:  

                             return_code = handleValidArgs(input_path, output_path,
                               return_code,encodeOutput);/*perform main functionality */
                             break;
            case SAME_FILE:
                            fprintf(standardout, 
                              "The output file %s was not created. Same as input file.\n", 
                              output_path);
                            break;
            case FILE_DNE:
                            fprintf(standardout, 
                              "The input file %s does not exist.\n", input_path);
                            break;
            default:
                            fprintf(standardout, "An unknown error occurred\n");
                            break;
        }
    } else {
        /* Alert the user what was not set before quitting. */
        if(input_path == NULL) {
            fprintf(standardout, "INPUT_FILE was not set.\n");
        }
        if(output_path == NULL) {
            fprintf(standardout, "OUTPUT_FILE was not set.\n");
        }
        /* Print out the program usage*/
        USAGE(argv[0]);
    }
    return return_code;
}


/* Function that checks if the arguments are valid 
 * @param const char *input_path: the input_path string
 * @param const char *output_path: the output_path string
 * @return : the return code
 */
int validate_args(const char *input_path, const char *output_path){
    /*Declare Var*/
    int return_code = FAILED; /* in header file FAILED = 3*/

    if(input_path != NULL && output_path != NULL) {  /* Make sure both strings are not NULL */
        /* Check to see if the the input and output are two different files. */
        if(strcmp(input_path, output_path) != 0) {
            struct stat sb; /* Check if input file exists */
            memset(&sb, 0, sizeof(sb) + 1); /* zero out the memory of one sb plus another */

            /* now check to see if the file exists */
            if(stat(input_path, &sb) == -1) {
                if(errno == ENOENT) { /* something went wrong */
                    return_code = FILE_DNE; /* File does not exist. */
                } else {
                    perror("NULL"); /* No idea what the error is. */
                }
            } else {

                struct stat sb2; /* To check the second file path*/
                memset(&sb2, 0, sizeof(sb2) + 1); /*zero out the memory */

                if(lstat(output_path,&sb) ==-1){
                    return_code = VALID_ARGS;
                }else if( (sb.st_dev == sb2.st_dev) & (sb.st_size == sb2.st_size) & 
                  (sb.st_ino == sb2.st_ino) ){
                    return_code = SAME_FILE; /*Symbolic link*/
                }else return_code = VALID_ARGS;
                
            }

        }else{     
                return_code = SAME_FILE; /*Same input and output files*/
         
        }
    }
    return return_code;
}


/* Original Function that converts UTF 8 to UTF 16 
 * @param const int input_fd : input file descriptor
 * @param const int output_fd: output file descriptor
 * @param endianResult: determine to write in little endian (1) or big endian (0);
 * @return boolean : conversion success
 */
bool convert(const int input_fd,const int output_fd,int endianResult)
{
   bool success = false;
   if(input_fd >= 0 && output_fd >= 0){
        
        /*Declare var */
        unsigned char bytes[4];  /* UTF-8 encoded text can be @ most 4-bytes */
        auto unsigned char read_value;
        auto size_t count = 0;
        auto int safe_param = SAFE_PARAM;/* DO NOT DELETE, PROGRAM WILL BE UNSAFE */
        void* saftey_ptr = &safe_param;
        auto ssize_t bytes_read ; 
        bool encode = false;

        /* Read in UTF-8 Bytes */
        while((bytes_read = read(input_fd, &read_value, 1)) == 1) { /* read byte by byte*/
            /* Mask the most significant bit of the byte */
            unsigned char masked_value = read_value & 0x80; /*get the MSB of the byte read*/
            if(masked_value == 0x80) { /* If the byte starts with a 1.*/
                if((read_value & UTF8_4_BYTE) == UTF8_4_BYTE || /* & with 0XF0*/
                   (read_value & UTF8_3_BYTE) == UTF8_3_BYTE || /*  0xE0 */
                   (read_value & UTF8_2_BYTE) == UTF8_2_BYTE) { /* 0xC0 */
                    /* Check to see which byte we have encountered */
                    if(count == 0) {
                        bytes[count++] = read_value; /*Don't evaluate, wait for cont. bytes */
                    } else { 
                        /* we have lead and continuation bytes. Hit new lead byte, evaluate*/ 
                        /* Set the file position back 1 byte */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                            /*Unsafe action! Increment! */
                            safe_param = *(int*)++saftey_ptr;
                            /* failed to move the file pointer back */
                            perror("NULL");
                            goto conversion_done;
                        }
                        /* Encode the current values into UTF-16LE */
                        encode = true;
                    }
                } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                    /* Add the continuation byte, pending evaluation and encoding */
                    bytes[count++] = read_value;
                }
            } else { /* if MSB doesn't start with a 1*/
                if(count == 0) {
                    bytes[count++] = read_value; /* We have a new US-ASCII : Evaluate*/
                    encode = true;
                } else {
                    /* Found an ASCII character but theres other characters
                     * in the buffer already.
                     * Set the file position back 1 byte.
                     */
                    if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                    	/*Unsafe action! Increment! */
                        safe_param = *(int*) ++saftey_ptr;
                        /* failed to move the file pointer back */
                        perror("NULL");
                        goto conversion_done;
                    }
                    /* Encode the current values in the buffer into UTF-16LE */
                    encode = true;
                }
            }
            /* If its time to encode do it here */
            if(encode){
                int i, value=0;
                bool isAscii = false;
                for(i=0;i<count;i++){
                  if(i==0){            
                    /* Masking to get the LSB of the bytes */ 
                    if((bytes[i] & UTF8_4_BYTE) == UTF8_4_BYTE) {
                       value = bytes[i] & 0x7; /*MSB = 11110 Grab the last 3 bits */
                    } else if((bytes[i] & UTF8_3_BYTE) == UTF8_3_BYTE) {
                       value =  bytes[i] & 0xF;/*MSB = 1110 Grab the last 4 bits */
                    } else if((bytes[i] & UTF8_2_BYTE) == UTF8_2_BYTE) {
                       value =  bytes[i] & 0x1F; /*MSB =110 Grab the last 5 bits  */
                    } else if((bytes[i] & 0x80) == 0) {
                       value = bytes[i]; /* Value is an ASCII character */
                       isAscii = true;
                    } else {
                        goto conversion_done; /* Marker byte is incorrect */
                    } 
                  }else{/* Going to the subsequent bytes */                    
                     if(!isAscii) {
                       /*value is from the previous bytes */
                       value = ((value << 6) | (bytes[i] & 0x3F));/* bytes[i] is current byte */ 
                     }else{
                       /* How is there more bytes if we have an ascii char? */
                       goto conversion_done;
                     }
                  }
                }

                /* Handle the value if its a surrogate pair*/
                if(value >= SURROGATE_PAIR){
                   bool writeSuccess = writeCodepointToSurrogatePair
                     (output_fd,value,endianResult); 
                   if(!writeSuccess) return success;
                } else{
                    /* write the code point to file */
                    if(!safe_write(output_fd, &value, CODE_UNIT_SIZE,endianResult)) return success;
                }
                /* Done encoding the value to UTF-16LE */
                encode = false;
                count = 0;
            }
        }
        /* If we got here the operation was a success! */
        success = true;
    }
    conversion_done:
    return success;
}


/* A function that converts from UTF16 LE or UTF16BE to UTF8
 * 0 means we're converting from UTF16BE 
 * 1 means we're converting from UTF16LE
 */
bool convertUTF16_UTF8(const int input_fd, const int output_fd, int endianness){
    /*Check for invalid file descriptor*/
    if(input_fd<0){
        perror("error: input_fd closed");
        return false;
    } else if (output_fd<0){
        perror("error: input_fd closed");
        return false;
    }
    /*Declare Var*/
    bool success = false;
    int bytes_read = 0;
    int codeUnitOne = 0;
    int codeUnitTwo = 0;
    int codepoint = 0;
    bool continueSearchSecondSurrogate = false;

    /* bytes_read = two byte hex values, unless no more utf16 bytes, then is -1 */
    while( (bytes_read = readUTF16TwoByte(input_fd)) != -1 ){ 
        /*Search for 1st code unit */
        if(!continueSearchSecondSurrogate){ 
            if(endianness ==1) /*Little Endian */
                codeUnitOne = UTF16TwoByteFlip(bytes_read);/*Flip Bytes since it is Little endian */
            else codeUnitOne = bytes_read; 
            /*Do we need to look for second pair?*/
            if(checkForSurrogatePair(codeUnitOne) == true){ 
                continueSearchSecondSurrogate=true;
                continue;
            }else codepoint = codeUnitOne; /*We have a code unit to evaluate */          
        }

        /*Search for second code unit */
        else{
            if(endianness ==1){ /*Little Endian*/
                codeUnitTwo = UTF16TwoByteFlip(bytes_read); 
                /*Flip Bytes again for little endian*/ 
            }
            else codeUnitTwo = bytes_read;

            continueSearchSecondSurrogate = false; /*Completed search for code units*/
            codepoint = generateCodePointFromSurrogatePair(codeUnitOne,codeUnitTwo);
        }
       int utf8Bytes = utf8FromCodePoint(codepoint); /*Convert codepoint to utf8 */
       int written = writeUTF8Bytes(output_fd,utf8Bytes); /*write utf8 bytes to file */
       if(written < 0) { 
           printf("ERROR writing in convertUTF16 to 8");
           success = false; 
           return success;

       }else success = true; /*Exit Early Read Failure */
    }
    return success;
}


/* A function that converts little endian UTF16 to big endian, and vice versa depending on input. 
 * @param input_fd: input filedescriptor
 * @param output_fd : output file descriptor
 * @return boolean : Successful or not. 
 */
bool convertUTF16BigLittle(const int input_fd, const int output_fd){
    /*Check for invalid file descriptor*/
    if(input_fd<0){
        perror("error: input_fd closed");
        return false;
    } else if (output_fd<0){
        perror("error: input_fd closed");
        return false;
    }

    /*Declare Var*/
    bool success = false;
    int bytes_read = 0;
    int codeUnit=0;
    int byteOne=0;
    int byteTwo=0;

    /* bytes_read = two byte hex values, unless no more utf16 bytes, then is -1 */
    while((bytes_read = readUTF16TwoByte(input_fd)) != -1 ){ 
        /*Grab a code unit and flip it */
        codeUnit = UTF16TwoByteFlip(bytes_read); /*Flip Bytes for the code unit*/

        /*Create bytes*/
        byteOne = codeUnit&0xFF00; /*Assign byte one*/
        byteTwo = codeUnit &0xFF; /*Assign byte two*/
        byteOne = byteOne>>8; /*Shift One byte over to isolate*/
          
        /*Writing byte one */
        int written = write(output_fd,&byteOne, 1); 
        if(written < 0) { 
            printf("ERROR writing in convertUTF16 Little/Big Endianness");
            success = false; 
            return success;
        }else success = true; /*Exit Early Read Failure */

        /*Writing Byte Two */
        written = write(output_fd,&byteTwo, 1); 
        if(written < 0) { 
            printf("ERROR writing in convertUTF16 Little/Big Endianness");
            success = false; 
            return success;
        }else success = true; /*Exit Early Read Failure */
    }
    return success;
}

/* Function that checks Endianness of the machine 
 * @param returns: 0 for big endian, 1 for little endian machine
 */
int checkEndian(){
  int checkEndian = 1;
  char *checkEndianPtr= ((char*) &checkEndian);
  int endianResult = ((int) (*checkEndianPtr));
  if(endianResult==0){
    return 0; /*This is a big endian machine */
  }else return 1; /* This is a little endian machine */ 
}
 
                 
/* Function that takes a input file descriptor and determines its BOM 
 * @param input_string: the string of the file name
 * @param return :  -1= invalid, 0 = utf8 , 1 = utf16le, 2 = utf16be
 */
int identifyEncoding(char* input_string){
    /* Declare var */
    int file_id = -1; /*File*/
    int bomEncoding = -1; /* return value -1 invalid, 0 utf8, 1 utf16le, 2 utf16be */
    
    /*Open file */
    file_id = open(input_string,O_RDONLY);
    if(file_id<0) return bomEncoding; /*return early*/
    
    /*reading open file */
    int bomBytes=0; /*entire BOM */
    int byteVal = 0; /*one byte */
    int bytes_read=0; /*check if read one byte */
    int count = 0; /*counter up to 4*/
    while(  (  (bytes_read=read(file_id,&byteVal,1)  )==1) & (count<3)){ /*Executes 3 times only */
        bomBytes= bomBytes<<8; /* Shift before mask */
        bomBytes= (bomBytes | byteVal); 
        count++;
    }
    if(bomBytes ==0xEFBBBF) bomEncoding = 0;
    else{ 
        bomBytes = bomBytes >> 8; /* Check for only 2 bytes */
        if(bomBytes == 0xFFFE) bomEncoding = 1;
        else if (bomBytes == 0xFEFF) bomEncoding = 2;
    }
    
    /*Close file */
    if(file_id >=0) close(file_id);

    /*return the encoding*/
    return bomEncoding;
}


/* Function that handles the valid args inputs into the program 
 * @param input_path : pointer to the string input path
 * @param output_path : pointer to the string output path
 * @param return_code_initial: inital return code value
 * @return : return code (if modified)
 */
int handleValidArgs(char* input_path, char* output_path,int return_code_initial, char *encodeFormat){

    /*Declare the vars */
    int return_code= return_code_initial;
    int input_fd = -1, output_fd = -1;
    bool success = false;
    bool filePrefixed=false; /*if prefix the output file is successful*/
    int inputFormat=-1;
    int outputFormat = -1;

    /*Determine the input file type */
    inputFormat = identifyEncoding(input_path);
    if(inputFormat ==-1) exit(EXIT_FAILURE); /*Exit Early*/

    /*Determine the valid output type */
    if((strcmp(encodeFormat,"UTF-16BE"))==0) outputFormat = 2;       /* UTF-16BE */
    else if((strcmp(encodeFormat,"UTF-16LE"))==0) outputFormat =1;  /* UTF-16LE */
    else if((strcmp(encodeFormat,"UTF-8"))==0) outputFormat =0;   /* UTF-8" */
    else exit(EXIT_FAILURE); /*Exit Early*/
    
    /*Check for invalid input-->output combinations */
    if(inputFormat ==outputFormat) {
        success = copyFile(input_path,output_path);
        return success;
    } /* We just read a utf8 input file */

    /* Attempt to open the input file */
    input_fd = open(input_path, O_RDONLY);
    if(input_fd < 0) {
        printf("Failed to open the file %s\n", input_path);
        perror(NULL);
        goto conversion_done;
    }
    /*Determine BOM Offset*/
    if(inputFormat ==0){ 
        /*Move the file descriptor for UTF8*/
        lseek(input_fd,3,SEEK_CUR);

    }else{ /*inputFormat must be >0 for LE or BE*/
        /*Move the file descriptor for UTF8*/
        lseek(input_fd,2,SEEK_CUR);
    }

    /* Delete the output file if it exists; Don't care about return code. */
    unlink(output_path);

    /* Attempt to create the file */
    output_fd = open(output_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    
    /*Failed Open */
    if(output_fd < 0) {
        /* Tell the user that the file failed to be created */
        printf("Failed to open the file %s\n", input_path);
        perror(NULL);
        goto conversion_done;
    }

    /*If output_fd open, prefix the file with the outputFormat BOM*/
    filePrefixed = prefixByteOrderMarkings(output_fd,outputFormat); /*prefix the BOM */
    if(filePrefixed ==false){
        perror("Failed File Prefix");
        return false;
    }

    /*Determine conversion type, then convert*/
    if( (inputFormat == 0) & (outputFormat == 1) )
        success = convert(input_fd, output_fd,1); /*Convert from utf8 to utf16LE */
    else if( (inputFormat==0) & (outputFormat ==2) ) 
        success = convert(input_fd, output_fd,0); /*Convert from utf8 to utf16BE */
    else if( (inputFormat==1) & (outputFormat ==0) ) 
        success = convertUTF16_UTF8(input_fd, output_fd, 1); /*Convert from utf16LE to utf8 */
    else if( (inputFormat==1) & (outputFormat ==2) ) 
        success = convertUTF16BigLittle(input_fd, output_fd); /*Convert from utf16LE to utf16BE */
    else if( (inputFormat==2) & (outputFormat ==0) ){
        success = convertUTF16_UTF8(input_fd, output_fd, 0); /*Convert from utf16BE to utf8 */
    }
    else if( (inputFormat==2) & (outputFormat ==1) )
        success = convertUTF16BigLittle(input_fd, output_fd); /*Convert from utf16BE to utf16LE */
    else exit(EXIT_FAILURE); /*Should never reach this statement unless inputFormat is incorrect*/

/*Exit Code */
conversion_done:
    if(input_fd >= 0)
        close(input_fd); 
    if(output_fd >= 0)
        close(output_fd);     
    if(success) {
        return_code = EXIT_SUCCESS;
    }else {
        /* Conversion failed; clean up */
        unlink(output_path); /* deletes the file output_path.*/
        return_code = EXIT_FAILURE;/* Just being pedantic... */
    }
    return return_code;
}


bool copyFile(const char* input_path,const char* output_path){
    int input_fd =-1,output_fd = -1;
    input_fd = open(input_path,O_RDONLY);
    if(input_fd < 0 ) return false;
    /* Delete the output file if it exists; Don't care about return code. */
    unlink(output_path);
    output_fd = open(output_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(output_fd < 0) return false;
    
    int bytes_read = 0;
    int byteVal = 0;
    bool written = false;
    while( (bytes_read = read(input_fd, &byteVal,1)) == 1){
        written = write(output_fd, &byteVal,1);
        if(written<0){
            written = false;
            perror("Unable to copy the file");
            break;
        }else written = true;
    } 
    close(input_fd);
    close(output_fd);
    return written;
}

/* A function that determine if the code unit is part of a surrogate pair
 * Assumes that everything is in big endian form, must reverse if start as Little end.
 * @param codeunit: two byte unit
 * @return: whether this code unit is part of a surrogate pair
 */
bool checkForSurrogatePair(int codeunit){
    if(codeunit <0xD800) return false;
    else return true;
}

/* Function that writes a value of certain size to a file descriptor
 * @param int output_fd: the file descriptor for file to be written to
 * @param void* value: pointer to the value to be written
 * @param size_t size: value of the size
 */
bool safe_write(int output_fd, int *value, size_t size, int endianResult)
{
    bool success = true;
    ssize_t bytes_written=0;
    int wholeVal= *value;
    int secondByte = wholeVal & 0xFF;
    int firstByte = ((wholeVal &0xFF00) >>8);

    if(endianResult==0){
    /*Encode big endian*/
        if((bytes_written = write(output_fd, &firstByte, size)) != size) {
            /* The write operation failed */
            fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size,
              bytes_written);
            success = false;
        }
        if((bytes_written = write(output_fd, &secondByte, size)) != size) {
            /* The write operation failed */
            fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size,
              bytes_written);
            success = false;
        } 
 
    }else if(endianResult ==1){
      /* Encode little endian */
      if((bytes_written = write(output_fd, &secondByte, size)) != size) {
            /* The write operation failed */
            fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size,
              bytes_written);
            success = false;
       }
       if((bytes_written = write(output_fd, &firstByte, size)) != size) {
            /* The write operation failed */
            fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size,
              bytes_written);
            success = false;
        } 
    }
    return success;
}

/* A function that reads UTF-16 Two Bytes individually. Assuming that 
 * the file descriptor is pointed after the BOM Value. The returned byte pair takes
 * no regard for the endianness of UTF16-BE or UTF16-LE.
 * Preconditions: filedescriptor must be opened already.  
 * @param const int fileDescriptor: The file descriptor of the input utf16 file
 * @return int: the conjoined hex value of the two bytes. 
 */
int readUTF16TwoByte(const int fileDescriptor){
    /*Declare var */
    int hexBytePair = -1; /*conjoined value of the byte pair*/
    int i; 
    int bytesRead =0;
    int count=0; 
    int byteValue = 0; /*Stores the individual byte */
    /*Read two bytes */
    for(i =0; i<2;i++){  /*Executes twice*/
        if( (bytesRead = read(fileDescriptor, &byteValue,1)) ==1){ /*Read one byte*/
            count++; 
            if(i == 0) hexBytePair = 0;
            hexBytePair = hexBytePair <<8; /*Shift one byte to the left, think binary */
            hexBytePair = (hexBytePair|byteValue);/* Capture the byte */
            
        }
    }
    if(count < 2) return -1; /* Wasn't able to read a whole code unit pair, return error */
    return hexBytePair; /* Else return two byte value */
}


/* A function that takes a pair of bytes and flips the order of bytes within the pair.
 * Effectively, it changes the endianness of a byte-pair 
 */
int UTF16TwoByteFlip(int bytePair){
    int byte1 = ((bytePair&0xFF00) >>8); /*Value of first byte */
    int byte2 = (bytePair&0xFF); /*Value of second byte */
    return ( (byte2 <<8) | byte1 ); /*Reversed order */
}


/* A function that generates the 10 bit key from the LSB 
 * @param lsbHex: A byte pair that is the LSB Key. 
 * @return: -1= error, else the value containing the 10 bits that is needed to decode the MSB
 */
int find10BitsFromLSB(int lsbHex){
    /* Goal: Find VL*/
    /*PseudoCode: lsbHex = 0xDC00 + vl */
    return lsbHex-0xDC00;
}

/*
 * A function that generates the codepoint for a UTF16 surrogate pair.
 * @param lsbPair: two bytes from the byte pair representing the LSB
 * @param msbPair: two bytes from the byte pair representing the MSB 
 * @return int : returns the original value of codepoint from a surrogate pair.
 */
int generateCodePointFromSurrogatePair(int msbPair, int lsbPair){
    int lsbKey = find10BitsFromLSB(lsbPair);   /*retrieve 10 bit key from the lsb */
    int codePoint = ((((msbPair-0xD800)<<10) | lsbKey ) + 0x10000); /*Generate the codepoint*/
    return codePoint;
} 



/* A function that takes a large codepoint over the value of 0x10000 and 
 * creates a surrogate pair out of it, and writes it to a file descriptor with the 
 * designated endianness.
 * @param output_fd: output file descriptor
 * @param codepoint: the function takes in the codepoint over 0x10000
 * @param endianness: determines whether the return is [MSB(big/little),LSB (big/little)]
 *                    1 = little endian , 0 is big endian. Matches checkEndian() return. 
 * @return bool: writing successful?
 */
bool writeCodepointToSurrogatePair(int output_fd, int codepoint, int endianness){
    /* Initialize Vars */
    int vprime = codepoint-SURROGATE_PAIR; 
    int w1 = (vprime >> 10) + 0xD800;     /*Surrogate MSB*/
    int w2 = ((vprime & 0x3FF) + 0xDC00); /*Surrogate LSB*/
    bool writeSuccess = false;

    /* write the surrogate pairs to file */
    writeSuccess = safe_write(output_fd, &w1, 1, endianness); 
    if(writeSuccess ==false) return writeSuccess;
    writeSuccess = safe_write(output_fd, &w2, 1, endianness);
    return writeSuccess;  
}


/* A function that takes a utf8 bytes and writes them sequentially into a file. 
 * @param output_fd: output file descriptor
 * @param utf8Bytes: the utf8 bytes, up to 4 bytes in the int.
 * @return int: writing successful? -1 is unsuccessful
 */
int writeUTF8Bytes(int output_fd, int utf8Bytes){
    /* Initialize Vars */
    int writeSuccess = -1;
    int bytesContained = 0;
    int writeByte = 0;

    /*Find out how many utf8Bytes you have to encode*/
    if( (utf8Bytes & 0xFF000000) != 0 ) bytesContained =4;
    else if ( (utf8Bytes & 0xFF0000) != 0 ) bytesContained =3;
    else if ( (utf8Bytes & 0xFF00) != 0 ) bytesContained =2;
    else bytesContained =1;

   

    /* write the utf8 bytes to file */
    int i;
    int shift;
 
    for(i=1;i<=bytesContained;i++){
        /*Getting the byte */
        shift = bytesContained -i; 
        int mask = (0xFF<< (8*shift));
        writeByte= (utf8Bytes & mask); /*first byte */
        writeByte = (writeByte >> (8*shift)); /* Shift back to isolate first byte */
        /*write byte*/
        writeSuccess = write(output_fd, &writeByte,1); /*Write one byte */
        if(writeSuccess < 0) return writeSuccess;
        
        /*Reset*/
        writeByte = 0;               /*Clear */
    }
    return writeSuccess;  
}


/*
 * A function that takes a codepoint value and returns 
 * the byte array for the codepoint in utf8.
 * @param : the codepoint to be converted
 * @return: the utf8 bytes
 */
int utf8FromCodePoint(int codepoint){
    /*Declare var */
    int bytesNeeded = utf8BytesNeededFromCodePoint(codepoint);
    int utf8Bytes = -1; /* Int value that holds up to 4 bytes for utf8*/   

    if((bytesNeeded <= 0) | (bytesNeeded > 4)){
        perror("Codepoint is invalid in converting to utf8Bytes");
        return utf8Bytes; /*Error Exit Early */
    }else if(bytesNeeded ==1){
        utf8Bytes = codepoint; /* As is */
    }else if(bytesNeeded ==2){
        /*Make byte B */
        int lastSixBits = (codepoint & 0x3F); /*Grab the last bytes */
        int byteB = (lastSixBits|0x80); /*prepend the binary: 10 as the byte*/

        /* Make byte A */
        int byteA = ((codepoint >> 6) &0x3F);/*shift, then grab the last six bits*/
        byteA = (byteA | 0xC0); /*prepend binary:110 for two bytes needed*/
        
        /*conjoin */
        utf8Bytes = ((byteA << 8) | (byteB));/*final result */
    }else if(bytesNeeded ==3){
        /*Make byte C */
        int lastSixBits = (codepoint & 0x3F);  /*continuation byte */
        int byteC = (lastSixBits|0x80); /*prepend the binary: "10" to continuation byte*/
        
        /*Make byte B*/
        lastSixBits = ((codepoint>>6)& 0x3F); /* Undo one shift, grab six bits*/
        int byteB = lastSixBits; /*copy value into continuation byte  (yy-xxxxxx)*/
        byteB = (byteB | 0x80);/* Prepend the binary: "10" to the byte (10-xxxxxx)*/

        /*Make leader byte A Format:*/
        lastSixBits = ((codepoint >> 12) &0x3F); /*Undo two shifts, grab six bits */
        int byteA = lastSixBits; /*Copy value into leader byte (yyyy-xxxx) */
        byteA = (byteA | 0xE0); /*prepend binary:1110 for three bytes needed*/
        
        /*Conjoin */
        utf8Bytes = ((byteA << 16) | (byteB <<8) | (byteC)); /*final result */
    }else { 
        /*Make byte D*/
        int lastSixBits = (codepoint & 0x3F); /* 0 shift , grab 6 bits */
        int byteD = (lastSixBits|0x80); /*prepend the binary: "10" to continuation byte*/

        /*Make byte C*/
        lastSixBits= ((codepoint >> 6)&0x3F); /*1 shift, grab 6 bits*/
        int byteC = lastSixBits; /*copy value into cont. byte */
        byteC = (byteC | 0x80); /* Prepend the binary: "10" to the cont. byte*/
 
        /*Make byte B*/
        lastSixBits= ((codepoint >> 12)&0x3F); /*2 shifts, grab 6 bits*/
        int byteB = lastSixBits; /* copy value into cont. byte */
        byteB = (byteB | 0x80); /*prepend binary:10 to the cont. byte*/
  
        /*Make byte A*/
        lastSixBits= ((codepoint >> 18)&0x3F); /* 3 shifts, grab six*/   
        int byteA = lastSixBits; /*copy the byte val into cont. byte*/
        byteA = (byteA | 0xF0); /*prepend binary:11110 to the byte*/
        
        /*Conjoin */
        utf8Bytes = ((byteA << 24)|(byteB <<16)|(byteC<<8)|(byteD)); /*final result */
    }
    return utf8Bytes;
}

/* A function that takes a codepoint and tells you how many utf8 bytes
 * are needed to represent it
 * @param codePoint (int) : the codepoint
 * @return : how many bytes needed to represent it in utf8
 */
int utf8BytesNeededFromCodePoint(int codepoint){
    if((codepoint >= 0x0) & (codepoint <= 0x7F)) return 1;
    else if((codepoint >= 0x80) & (codepoint <= 0x7FF)) return 2;
    else if((codepoint >= 0x800) & (codepoint <= 0xFFFF)) return 3;
    else if((codepoint >= 0x10000) & (codepoint <= 0x1FFFFF)) return 4;
    else return -1; /*Error*/
} 






/* If the value is less than 0x10000, it is same as code point:
 * Therefore check for the endianess of the encoding.
 *   If big endian,it is the same
 *    If little endian, reverse the byte order. We have the codepoint now.
 */


 /* If we have a surrogate pair, decode the surrogate pair to codepoint. 
  */

  /* With the code point, reverse the encoding to UTF8 */
    /* Determine the code point range */


    /* if between 0x0 -0x7F, no transformations */


    /* if between 0x80 and 0x7FF, 2 transformations:
       To get two bytes: 
       byte 1: grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 2: shift leftover to right by six bytes 
               grab the six bytes. Append 0x110 to front

    */
       
    /* if between 0x800 -0xFFFF 3 transformations 
       To get three bytes: 
       byte 1: grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 2: shift >> 6. 
               grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 3: shift >> 6 
               Append 0x1110 to front

    */
    
     /* if between 0x800 -0xFFFF 4 transformations 
       To get four bytes: 
       byte 1: grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 2: shift >> 6. 
               grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 3: shift >> 6. 
               grab the last six bytes with (val & 3F)
               Append 0x10 to the front
       byte 4: shift >> 6 
               Append 0x11110 to front

    */

    /* Encode the bytes since they're now in utf-8 */
    /* Insert the BOM */
    /* Add the encoded */


/* Function that takes a file descriptor and encodes the BOM to it 
 *
 */
bool prefixByteOrderMarkings(int fileDescriptor,int bomType){
    bool success = false;
    int bytesWrote=0;

    if(bomType ==0){ /*UTF-8* BOM */
        int bomValues[] = {0xEF,0xBB,0xBF};
        int i;
        int bomByte;
        for(i=0;i<3;i++){
            bomByte = bomValues[i];
            if((bytesWrote = write(fileDescriptor, &bomByte,1))==1) success = true;
            else success = false; 
        }
    }
    else if(bomType ==1){ /*UTF-16LE BOM */
        int bomValues[] = {0xFF,0xFE};
        int i;
        int bomByte;
        for(i =0;i<2;i++){
            bomByte = bomValues[i];
            if((bytesWrote = write(fileDescriptor, &bomByte,1))==1) success = true;
            else success = false; 
        }      
    }
    else if (bomType ==2){ /*UTF -16BE BOM */
        int bomValues[] = {0xFE,0xFF};
        int i;
        int bomByte;
        for(i =0;i<2;i++){
            bomByte = bomValues[i];
            if((bytesWrote = write(fileDescriptor, &bomByte,1))==1) success = true;
            else success = false; 
        }
    }
    return success;   
}

  
