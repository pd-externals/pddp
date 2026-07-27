
proc pddplink_open {obj filename dir} {
    if {[string first "://" $filename] > -1} {
        menu_openfile $filename
    } elseif {[file pathtype $filename] eq "absolute"} {
        menu_openfile $filename
    } elseif {[file exists [file join $dir $filename]]} {
        set fullpath [file normalize [file join $dir $filename]]
        set dir [file dirname $fullpath]
        set filename [file tail $fullpath]
        menu_doc_open $dir $filename
    } else {
        bell ;# beep on error to provide instant feedback
        set err [_ "\[pddplink\] ERROR file not found: %s" $filename]
        ::pdwindow::logpost ${obj} 1 "${err}\n"
    }
}
