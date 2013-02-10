PAWS
====

A utility for automatically pausing music when some other audio starts playing
and resuming the music afterwards.

Building instructions
---------------------
Install gcc, make, pkg-config and pulseaudio if necessary and then run

    $ make

in the directory to which paws has been downloaded.

Usage
-----

    $ paws [-v] [-p pause-cmd] [-r resume-cmd] [-i ignore-client]

### `-v`

Verbose. Print stuff when things happen.

### `-p pause-cmd`

Run `pause-cmd` when another application appears to have started playing audio.

### `-r resume-cmd`

Run `resume-cmd` when the applications that made us pause have stopped.

### `-i ignore-client`

Ignore any events that stem from a Pulseaudio client named `ignore-client`.
This will typically be the client name of your music player.

Example
-------

For Clementine, use the following flags:

    $ paws -p "clementine -u" -r "clementine -p" -i Clementine

To find out what client name to ignore for your music player, run:

    $ paws -v

If you fire up your music player and start playing some music at this point,
paws should print e.g.:

    Client 225 (Clementine) added its first input sink (# active clients = 1)

The text in parentheses is the name that you want to ignore.

Implementation notes
--------------------

The utility is written in C and uses Pulseaudio's asynchronous API. It listens
to certain Pulseaudio events, but will be idle for the most part.

The events that it listens to are those to do with clients (i.e. applications
that connect to the Pulseaudio server) and their input sinks, which are what
clients use to play audio.

The utility keeps track of any new input sinks and will run the pause command
when one appears. When the sinks that it keeps track of have been removed, it
will run the resume command.

It should be evident that the method used to determine if there is anything
playing may not always be accurate, but it seems to work pretty well.

Contact
-------
Olle Fredriksson - https://github.com/ollef
