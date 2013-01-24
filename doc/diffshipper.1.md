diffshipper(1) -- Floobits diff shipper. The mediocre fall-back solution for people with unsupported editors.
=============================================

## SYNOPSIS

`diffshipper -r ROOM_NAME -o ROOM_OWNER -u USERNAME -s API_SECRET [OPTIONS] PATH`


## DESCRIPTION

Join a Floobits room and monitor files in PATH for changes.


## OPTIONS

  * `--api-url URL`       For debugging/development. Defaults to https://floobits.com/api/room/
  * `--create-room`       Create a room and add PATH
  * `--delete-room`       Delete a room. Can be used with --create-room.
  * `-D`                  Enable debug output
  * `-h HOST`             For debugging/development. Defaults to floobits.com.
  * `-o OWNER`            Room owner
  * `-p PORT`             For debugging/development. Defaults to 3148.
  * `-r ROOMNAME`         Room to join
  * `--recreate-room`     Delete the room if it exists, then create it again
  * `--room-perms PERM`   Used with --[re]create-room. 0 = private, 1 = readable by anyone, 2 = writeable by anyone
  * `-s SECRET`           API secret
  * `-u USERNAME`         Username
  * `-v`                  Print version and exit


## EXAMPLES

`diffshipper -r testroom -o myuser -u myuser -s ve3aiCoo ~/code/collab_project/`

Join https://floobits.com/r/myuser/testroom/ and keep files in ~/code/collab_project/ synced with that room.
