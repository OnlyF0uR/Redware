# Redware
Proof-of-concept pythonic malware + command and control (backend) in [C](https://www.youtube.com/watch?v=tas0O586t80).

### Note
Some strings that are visibile in the code are already cyphered, if you would like to see the actual value then simply run
them through the decypher method in the utils file. Also if you don't like my shorthand variable and function names then I would suggest you to get over it.

### Commands
```
/screenshot <FINGERPRINT>
/shutdown <FINGERPRINT>
/keylogger <FINGERPRINT> <AMOUNT_OF_KEYS>
/info <FINGERPRINT>
/background <FINGERPRINT>
```