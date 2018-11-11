# HTML-hunter

## about
The point of an HTML hunter attack is to insert additional fields into a user’s browser when he or
she visits a login page (usually for a banking site, social networking site, or webmail site). To
the end user, the extra fields appear legitimate because they blend in with the rest of the login
form.

## HTML hunting with MITM
HTML hunter can be done with a traditional MITM (man-in-the-middle) attack, where a
malicious host positions itself on the network between the web server and the victim’s computer.
This position enables the attacker to replace or insert data into the server’s response
before it reaches the victim. Because of the complexities involving SSL and the requirement
of a unique network standpoint, the traditional MITM attack is least common. There are
two more prevalent methods, which include API hooking and IE DOM modification. 
