iPXE README File

Quick start guide:  
```
   cd src  
   make
```

Prompt usage: prompt [--key \<key\>] [--timeout \<timeout\>] [--variable \<variable\>] [\<text\>]  
variable syntax is equal to "set" command. For example:  
```
   prompt --variable=var:int16
``` 
After user prompts, you can use variable "var" via ${ }, show, clear, set etc. ASCII code of the read symbol will be stored in the variable.  

Added AVI video playing to console function, usage:
```
   console --picture=tftp://192.168.0.103/test.avi --keep
```
`--keep` is nessesary
MPEG-4 codecs (H264, AVC, XVID) are supported.
For any more detailed instructions, see http://ipxe.org

