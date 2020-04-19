# cuap-gateway
CUAP ( China Mobile USSD Application Protocol ) gateway is a CUAP implementation which provides an HTTP interface with which you can connect
your with any language of your choice so far it can do HTTP.

cuap-gateway communicates with ISP via config, data received are sent to HTTP backend via json payload as described below.

#### Building

Dependencies

​	Trantor    : https://github.com/an-tao/trantor
​	Drogon    : https://github.com/an-tao/drogon
​	Argparser: https://github.com/fmenozzi/argparser. I use a slightly modified version.

Running via Exodus binary:

   1. `$ ./cuap-gateway.exodus .`
      to extract app into current dir. Current dir now have bin/ data/ and bundles/

   2. `$ bin/cuap-gateway --config=gateway.json`  to run the App itself
      `bin/cuap-gateway --help` to see help message, but config file for control currently.

`gateway.json` looks like:

```
   {
      "app": {
         "mode": "gateway",
         "threads": 2
      },

      "gateway": {
         "host": "10.21.23.15",
         "port": 8727,

         "system-id"   : "system-id-goes-here",
         "password"    : "password-goes-here",
         "system-type" : "USSD",
         "welcome-page": "WIP",

         "client": {
           "url": "http://127.0.0.1:9980/"
         }
      }
   }
```


   `app` : application configuration

    mode: gateway | simple.
          "gateway" mode passes requests to client: string
          "simple"  mode is supposed to display whatever you have in welcome-page only : string [WIP]
    
    threads:  App threads to run: integer [WIP]

   `gateway` : USSDC gateway configuration

    host:   put the host given to you by ISP : string
    port:   Port                             : integer
    
      If you've read the CUAP Documention, the parts below will be familiar.
    
    system-id   : contact your Service Provider : string
          Specification says not more than 11 characters. Truncation will in the gateway if it exceeds.
    
    password    : contact your Service Provider : string
          Specification says not more than 9 characters. Truncation will in the gateway if it exceeds.
    
    system-type : "USSD" : string.
          system-type is "USSD" unless the Service Provider got other ideas.
          Specification says not more than 13 characters. Truncation will in the gateway if it exceeds.
    
    welcome-page: related to app.mode.simple: string [WIP]

   `client` :  http backend related config

> url: http://ip:port/ of the HTTP Backend the Gateway will forward requests to : string
>          Example: "http://127.0.0.1:9980/" : string



#### How it works.

1. BIND:

    When cuap-gateway successfully login, it sends below payload to HTTP backend:

        { "command": 103, "length": 31, "system_id": "your system-id" }

     Backend responds with:  { "status": 200, "message": "Success" }
     Non 200 indicates error.

2. When user make a USSD request, say *142#.

   

   2a. This shows the details of the pdu coming in from USSDC.

    `{ "sid": "0x0001db02", "rid": "0xffffffff", "service_code": "*142", "operation": "USSR", "msisdn": "80xxxxxxxxxx" }`
   
   
   
>  sid:          Sender ID
   >  rid:          Receiver ID
   >  service_code: code typed, # is removed in the specs
   >  operation     :  represents type of operation to be performed
   
   >       USSR = message sent from an SP to the USSDC.
   >              Comes with Begin message, i.e when user types the code and press send. As we can see above it's "USSR"
   >      
   >       PSSR = process unstructured supplementary service data request (PSSR).
   >              Dialog with input.
   >       USSN =
   >          Unstructured supplementary service data NOTIFY (USSN), message sent from an SP to the USSDC.
   >          This is the DIALOG without input, just a display dialog hence the name NOTIFY.

   2b. Below is what gets send to the HTTP backend:

​			`{ "command": 111, "sid": "0x00013731", "length": 0, "msisdn": "80xxxxxxxxxx", "content": "*142" }`

> 	command: CAUP PDU Command ID, refer to the CUAP docs for this. Convert the HEX to Deicimal for usage here in your json payload.
> 	length : CUAP PDU Command Length
> 	sid    : Sender ID
> 	msisdn : Sender's Phone number
> 	content: What user typed



HTTP Backend responds with:

​	 `{ "command": 112, "op_type": 1, "msisdn": "80xxxxxxxxxx", "content": "Welcome to Our USSD Service....." }`

>     op_type: is the operation type discussed above.
>     
>     USSR = 1,
>     PSSR = 1, // Yes also one, except the direction is reversed it is sent to caup-gateway -----> USSDC (ISP)
>     USSN = 2,
>

   

2c. When user selects an option, u get:
       ` { "command": 112, "sid": "0x00013731", "length": 66, "msisdn": "80xxxxxxxxxx", "content": "Option 1" }`

​	

​		HTTP Backend can reply using below format:

​		`{ "command": 113, "op_type": 2, "msisdn": "80xxxxxxxxxx", "content": "What ever Option one is to fetch" }`

>     	command : 113, is USSDEnd. Refer to CUAP Documention from ISP. Remember to convert to decimal.
>         op_type : 2  , is USSN. As described above in (2a)
>



​	  If you want to keep asking for input, you'd use 112 (CONTINUE) and 1 (PSSR). For example:

​			`{ "command": 112, "op_type": 1, "msisdn": "80xxxxxxxxxx", "content": "Please enter input" }`



 2d. When a user press the Cancel/End button on the phone, USSDC (ISP) sends an abort. `cuap-gateway` will send the below to the HTTP Backend for processing.

​	`{ "command": 114, "sid": "0x00013731", "length": 20 }`

>      With sid you can know which msisdn aborted, since it's included in the payload shown above in (2b).
>