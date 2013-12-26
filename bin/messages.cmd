/* REXX: messages.cmd, part of Elm for OS/2 */

maildir = 'c:\uupc\mail'

parse arg folder rest

if rest \= "" 
then do
  say ''
  say 'Usage: messages {folder-name}'
  exit 1
end

if folder = ""
then do
  fname = maildir || '\' || value('LOGNAME',,'OS2ENVIRONMENT')
  optional = 'in your mailbox'
end 
else do
  fname = folder
  optional = 'in folder ' || folder
end

queue = rxqueue('create')
call rxqueue 'set', queue

'@egrep -c "^From " ' || fname || ' | rxqueue 'queue

if queued() > 0
then parse pull mcount
else mcount = -1

call rxqueue 'delete', queue

if mcount = -1
then exit 0

say ''

if mcount = 1
then say 'There is 'mcount' message 'optional'.'
else say 'There are 'mcount' messages 'optional'.'

exit mcount
