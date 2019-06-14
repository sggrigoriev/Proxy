Release 1.2.9

WUD/wud_actions

    Some set of wud functions. Another garbage. (Why all garbage folders are sorted to the top?!)

    1. wa_alarm -               WUD timer service oriented to registered child processes.
                                Now it is Proxy & Agent

    2. wa_manage_children -     Currently used just for Proxy start. The Agent start is disabled.
                                To use it each child process should create the file with own pid number.
                                Used in full strength with Agent emulator

    3. wa_reboot -              WUD timer service oriented to registered child processes.
                                Now it is Proxy & Agent