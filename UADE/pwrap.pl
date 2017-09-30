#!/usr/bin/perl -w
######################################
# pwrap.pl v1.1.1 by Meleth in 2002. #
#                                    #
# A perl frontend for UADE           #
######################################

use strict;
use Term::Cap;
use vars qw/@mod @hold $uade @player @ext $check $sub $repeat $random $cls $play $play_list $l $f2 $current $mymods $pan $pl $bindir $ftpldir $ftprdir $user $pass $site $proxy $firewall $i @pids @index $ftp $down/;
$down = 0;
$| = 0;
$cls = qx/clear/;
use Cwd;
use POSIX ":sys_wait_h";
use POSIX qw(_exit);
use Net::FTP;
$uade="/usr/local/share/uade";
$mymods="$ENV{HOME}";
$bindir="/usr/local/bin";

$SIG{CHLD} = sub { REAPER(); };
$pan = "50";
$check = "0";
$sub = "0";
$repeat = "0";
$random = "0";
$i = "0";
$ftpldir = "/tmp";
$ftprdir = "/pub/aminet/mods";
$user ="ftp";
$pass ="pwrap\@uade.com";
$site ="ftp.sunet.se";
$proxy ="FALSE";
$firewall ="";

sub REAPER {
    my $stiff;
    my $x=0;
    my $t=0;
    my $pid = waitpid(-1, &WNOHANG);
    while (($stiff = waitpid(-1, &WNOHANG)) > 0) {
    }
    $SIG{CHLD} = \&REAPER;
    if (defined $pids[0]) {
	foreach my $bah (@pids) {
	    if ($bah eq $pid) {
		splice(@pids,$x,1);
		$t=1;
		$down--;
		my $file = rfile();
		drawmenu();
		print "Downloaded and enqueued $file\n";
	    }
	    else {
		$x++;
	    }
	}
    }
    if ($t == 0) {
	autoskip();
    }
}

if (@ARGV == 0) {
    &opendefpl();
    &readformats();
    &menu();
}

elsif ($ARGV[0] eq "--tk") {
    &opendefpl();
    &readformats();    
    &tk();
}

elsif ($ARGV[0] eq "--help" or $ARGV[0] eq "-h" or $ARGV[0] eq "--h") {
    &usage();
}
elsif ($ARGV[0] eq "--pl"){
    print("Reading playlist.\n");
    if (-f $ARGV[1]){
	open(PLAYFILE, "$ARGV[1]") or die "Can't open playfile: $!\n";    
	foreach (<PLAYFILE>) {
	    my $file = openf($_);
	    chomp $file;
	    push @mod, $file;
	}
        close PLAYFILE;
	&readformats();
	smod();
	&play($current);
	&menu();
    }else {
	&usage();
    }
}
else {
    foreach my $file (@ARGV){
	push @mod, openf($file);
    }
    &readformats();
    smod();
    &play($current);
    &menu();    
}

sub tkplay {
    my $file = $play_list->get('active');
    return if (!$file);
    $sub = 0;
    $play->configure(text => '[]');
    if (defined $current) {
	unshift @mod, $current;
    }
    if (defined $hold[0]){
	for (my $x=0;$x <= $#hold;) {
	    my $temp = pop @hold;
	    unshift @mod, $temp;
	}
    }
    until ($mod[0] eq $file) {
	my $temp = shift @mod;
	push @hold, $temp;
    }
    $current = shift @mod;
    if ($check == 1){
	stop();
    }
    else {
	play($current);	 
    }
}

sub tkadd {
    my $in = "@_";
    if (-d $in) {
	opendir(DIR, "$in");
	my @files = readdir DIR;
	closedir DIR;
	foreach my $entry (@files){
	    my $temp = "$in/$entry";
	    if (-f $temp)  {
		my $t2 = openf($temp);
		if ($t2 ne "0") {
		    push @mod, "$t2";
		}
	    }
	}
	if (!defined $current){
	    smod();
	}
	uptk();
	return;
    }
    if (-f $in) {
	my $t2 = openf($in);
	unshift @mod, "$t2";
    }
    if (!defined $current){
	smod();
    }
    uptk();
}

sub pladd {
    my $in = "@_";
    if (-f $in) {
	clist();
	if (defined $mod[0]) {
	    pop @mod;
	}
	open(PLAYFILE, "$in") or die "Can't open playlist: $!\n";
	foreach (<PLAYFILE>) {
	    chomp $_;
	    push @mod, "$_";
	}
	close PLAYFILE;
	uptk();
	return "Loaded playlist: $in";
    }
    else {
	return "Can't open playlist.";
    }
}

sub uptk {
    if ((Tk::Exists($play_list)) == 1){
	my $ind;
	$play_list->delete(0,'end');
	$play_list->insert('end',@hold,$current, @mod);
	if (defined $current) {
	    my $temp = ($play_list->size)-1;
	    for (my $x=0;$x <= $temp;$x++){
		if (($play_list->get($x)) eq $current) {
		    $ind = $x;
		}
	    }
	    $play_list->yview($ind);
	    $play_list->selectionSet($ind);
	}
    }
    if (defined $current){   
	$l->configure(text => parsef($current)."\nSubsong: ".$sub);
    }
}


sub tk {
    use autouse 'Tk';
    use autouse 'Tk::FileSelect';
    require Tk::HList;
    require Tk::Balloon;
    require Tk::FileSelect;
    my $tkplist;
    my $top = MainWindow->new();
    my $image = $top->Pixmap(-file => "$uade/uadelogo.xpm");
    $top->title ("pwrap.pl v1.1.1");
    my $logo = $top->Label (anchor => 'n');
    $logo->pack(side => 'top');
    $l = $top->Label (text   => "Welcome to pwrap\n Subsong: ".$sub,
		      anchor => 'n',                
		      relief => 'sunken');
    $l->pack(side => 'top');
	my $scaler = $top->Scale(orient       => 'horizontal',
            from         => 0,  
            to           => 200,
            tickinterval => 25,
            label        => 'Panning',
            font         => '-misc-fixed-medium-r-normal--9-60-100-100-c-50-iso8859-1',
            length       => 200,
            variable     => \$pan,
            )->pack(side => 'top',
                    fill => 'x');
    my $f0 = $top->Frame->pack(side => 'top');
    my $f1 = $top->Frame->pack(side => 'bottom'); 
    $play = $f0->Button(
			 text    => '|>', 
			 command => sub {
				 if ($play->cget('text') eq "|>") { 
				     $play->configure(text => '[]');
				     play($current);
			     }
			     else {
				 $l->configure(text => 'Stopped');
				 $play->configure(text => '|>');
				 stop(5);  
			     }
			 }
			 );
    

    my $eject = $f0->Button(
			    text => '^',
			    command => sub {
		        my $f = $top->FileSelect(-verify => [],
						 -directory => $mymods,
			      	    -title => 'Select a file or dir to add');
			my $file = ($f->Show);
			if (defined $file) {
			tkadd($file);
		    }
		    }
			    );

    my $next = $f0->Button(
			   text => '>|',
			   command => sub {
			       nextmod()});
    my $back = $f0->Button(
			    text => '|<',
			    command => sub {
				back()});
    my $nsub = $f0->Button(
			    text => '>',
			    command => \&nsub);
    my $psub = $f0->Button(
			    text => '<',
			    command => \&psub);
    my $rep = $f1->Checkbutton ( variable  => \$repeat, 
				  text     => 'Repeat');
    my $ran = $f1->Checkbutton ( variable  => \$random, 
				  text     => 'Random');

    $pl = $f1->Checkbutton ( variable => \$tkplist,
				text     => 'Playlist',
				command  => sub {
    	    if ($tkplist == 1){
		$f2 = MainWindow->new();
		$f2->title ("Playlist");
		my $f5 = $f2->Frame->pack(side => 'top');
		$play_list = $f5->Listbox("width" => 0, "height" => 15,
					  selectmode => 'single',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					  -selectbackground => 'white',
  					  )->pack(side => 'left');
		$play_list->insert('end',@hold, $current, @mod);
		my $scroll = $f5->Scrollbar(orient  => 'vertical',
					    width   => 10,
					    command => ['yview', $play_list]
					    )->pack(side => 'left',
						    fill => 'y',
						    padx => 10);
		$play_list->configure(yscrollcommand => ['set', $scroll]);
		$play_list->bind('<Double-1>', \&tkplay);
		my $f7 = $f2->Frame->pack(side => 'bottom');
		my $f4 = $f2->Frame->pack(side => 'bottom');

		my $cpl = $f4->Button ( text => 'Clear Playlist',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					 command => sub {
					     clist();
					     uptk()});
		$cpl->pack(side => 'left');
		my $rur = $f4->Button ( text => 'Remove Selected',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					 command => sub {
					 my $x=0;
					 my $s = $play_list->get('active');
					 chomp $s;
					 if (defined $current) {
					     if ($current eq $s) {
						 $current = $mod[0];
					     }
					 }
					 if (defined $mod[0]) {
					     foreach my $bah (@mod) {
						 if ($bah eq $s) {
						     splice(@mod,$x,1);
						 }
						 else {
						     $x++;
						 }
					     }
					 }
					 if (defined $hold[0]) {
					     $x=0;
					     foreach my $look (@hold) {
						 if ($look eq $s) {
						     splice(@hold,$x,1);
						 }
						 else {
						     $x++;
						 }
					     }
				       	 }
					 $play_list->delete('active');
				     });

		$rur->pack(side => 'left');
		my $sort = $f7->Button ( text => 'Sort',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					 command => sub {
					     if (defined $hold[0]){
						 for (my $x=0;$x <= $#hold;) {
						     my $temp = pop @hold;
						     unshift @mod, $temp;
						 }
					     }
					     @mod = sort @mod;
					     uptk()});
		$sort->pack(side => 'left');
		my $add1 = $f7->Button(	text => 'Add File/Dir',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					command => sub {
					    $eject->invoke();
					});
		$add1->pack(side =>'left');

		my $addpl = $f7->Button(	text => 'Add Playlist',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					command => sub {
					     my $file = $top->getOpenFile;
					     if (defined $file) {
					     pladd($file);
					 }
					});
		$addpl->pack(side =>'left');

		my $save = $f7->Button ( text => 'Save',
font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1',
					 command => sub {
					     my $file = $top->getSaveFile;
					     if (defined $file) {
					     savepl($file);
					 }
					 });
		$save->pack(side =>'left');
		uptk();
	}			     
	    else {
		$f2->DESTROY();
	    }
	}
    
				);
    my $balloon = $top->Balloon(font => '-misc-fixed-medium-r-normal--13-60-100-100-c-50-iso8859-1');
    $balloon->attach($play, -msg => "Play/Stop");    
    $balloon->attach($psub, -msg => "Previous Subsong");    
    $balloon->attach($back, -msg => "Previous Song");    
    $balloon->attach($next, -msg => "Next Song");    
    $balloon->attach($nsub, -msg => "Next Subsong");    
    $balloon->attach($rep, -msg => "Turn on repeat mode");    
    $balloon->attach($ran, -msg => "Play songs randomly");    
    $balloon->attach($pl, -msg => "Open/Close the playlist");    
    $balloon->attach($eject, -msg => "Add Files");    
    $balloon->attach($logo, -msg => "UADE - Only for real men");    
    $balloon->attach($l, -msg => "Created by Meleth in 2002");    
    $balloon->attach($scaler, -msg => "Panning values: 0=Stereo 100=Mono 200=Reverse Stereo NOTE: Will not get active until song change.");    
    $logo->configure (image => $image);
    $psub->pack(side =>'left');
    $back->pack(side =>'left');
    $play->pack(side =>'left');
    $next->pack(side =>'left');
    $nsub->pack(side =>'left');
    $eject->pack(side =>'left');
    $rep->pack(side => 'left');
    $ran->pack(side => 'left');
    $pl->pack(side => 'left');


    Tk::MainLoop();
    quit();
}

sub drawmenu {
    print $cls;
    print "------------------------------------------------------\n";
    if ($check == 0){
	print "5 Play";
    }else {
	print "5 Stop";
    }
    print "                  / Open Playlist\n";
    print "4 Back                  + Display Playlist\n";
    print "6 Next                  * Remove current from playlist\n";
    print "7 Previous Subsong      - Clear playlist\n";
    print "9 Next Subsong          , Save Playlist\n";
    if ($repeat == 0){
	print "1 Enqueue Directory     2 Repeat ON\n";
    }
    else {
	print "1 Enqueue Directory     2 Repeat OFF\n";
    }   
    if ($random == 0){
	print "3 Open Module           8 Random ON\n";
    }
    else {
	print "3 Open Module           8 Random OFF\n";
    }   
    print "p Panning               j Jump to module\n";
    print "\n";
    print "i Download index        d Download file\n";
    print "\n\n0 Quit\n";
    print "------------------------------------------------------\n";
    if (defined $current) {
	print "Position  : ". parsef($current) ."\n";
    }
    else {
    print "Position  : Nothing to play\n";
}
    print "Subsong   : $sub\n";
    print "Panning   : $pan\n";
    if ($repeat == 0){
	print "Repeat    : OFF\n";
    }
    else {
	print "Repeat    : ON\n";
    }
    if ($random == 0){
	print "Random    : OFF\n";
    }
    else {
	print "Random    : ON\n";
    }
    print "Downloads : $down\n";
print "------------------------------------------------------\n";
}   



sub menu {
    my $msg="";
    my $x=0;
    while ($x==0) {
	drawmenu();
	if (defined $msg) {
	print "$msg\n";
    }
	my $com = getone();

      SWITCH: {
	  if ($com eq "5") {
          if ($check == 0) {$msg = (play($current)); last SWITCH;}
       	  else             {$msg = "Stop"; stop(5); last SWITCH;}}
	  if ($com eq "4") {$msg = (back()); last SWITCH;}
	  if ($com eq "6") {$msg = (nextmod()); last SWITCH;}
	  if ($com eq "7") {$msg = (psub()); last SWITCH;}
	  if ($com eq "*") {$msg = (remcur()); last SWITCH;}
	  if ($com eq "9") {$msg = (nsub()); last SWITCH;}
	  if ($com eq "p") {$msg = (epan()); last SWITCH;}
	  if ($com eq "3") {$msg = (lmod()); last SWITCH;}
	  if ($com eq "-") {$msg = (clist()); last SWITCH;}
	  if ($com eq "2") {$msg = (repeat($repeat)); last SWITCH;}
 	  if ($com eq "i") {$msg = (getindex(1)); last SWITCH;}
 	  if ($com eq "d") {$msg = (ijump()); last SWITCH;}
	  if ($com eq "8") {$msg = (random($random)); last SWITCH;}
	  if ($com eq "0") {quit(); last SWITCH;}
	  if ($com eq "1") {$msg = (opdir()); last SWITCH;}
	  if ($com eq "j") {$msg = (jump()); last SWITCH;}
	  if ($com eq "/") {$msg = (openfile()); last SWITCH;}
 	  if ($com eq "+") {$msg = (dispfile()); last SWITCH;}
 	  if ($com eq ",") {$msg = (savefile()); last SWITCH;}
	  else             {$msg = "Unknown Command"; last SWITCH;}
      }
    }
}

sub epan {
    print "Panning values: 0=Stereo 100=Mono 200=Reverse Stereo\n";
    print "Enter panning [0-200]: ";
    chomp(my $temp = <STDIN>);
    if ($temp eq "") {
	return "Canceled";
    }
   if ($temp >= 0 && $temp <= 200) {
	$pan = $temp;
	return "Panning changed to: $pan";
    }
    else {
	return "Illegal value. Value must be between 0-200";
    }
}

sub jump {
    $i=0;
    print"Enter mod number [mod:sub] (:sub can be omitted) or search string: ";
    chomp(my $numb = <STDIN>);
    if ($numb eq "") {
	return "Canceled";
    }
    if ($numb =~ /[^0-9:]/) {
	print $cls;
	my $c=0;	
	if (defined $hold[0]) {
	    foreach (@hold){
		$c++;
		my $temp = parsef($_);
		if ($temp =~ /.*$numb.*/i) {
		    formprint($c,$temp);
		    my $p = pause();
		    if ($p eq "1") {
			return "Jump aborted";
		    }
		}
	    }
	}
	if (defined $current){
	    $c++;
	    my $temp = parsef($current);
	    if ($temp =~ /.*$numb.*/i) {
		formprint($c,$temp);
		my $p = pause();
		if ($p eq "1") {
		    return "Jump aborted";
		}
	    }
	}
	if (defined $mod[0]){
	    foreach (@mod){
		$c++;
		my $temp = parsef($_);
		if ($temp =~ /.*$numb.*/i) {
		    formprint($c,$temp);
		    my $p = pause();
		    if ($p eq "1") {
			return "Jump aborted";
		    }
		}
	    }
	}
	$i=0;
	print "\n";
	jump();    
    }
    else {
	if ($numb =~ /.*:.*/) {
	    $numb =~ /(.*):(.*?$)/;
	    my $num = $1;
	    $sub = $2;
	    if ($sub eq "") {
                $sub = 0;
            }
	    $num--;
	    jplay($num);
        }
        else {
           $numb--;
           jplay($numb);
        }
    }
}

sub jplay {
    my $file = "@_";
    if (defined $current) {
	unshift @mod, $current;
    }
    if (defined $hold[0]){
	for (my $x=0;$x <= $#hold;) {
	    my $temp = pop @hold;
	    unshift @mod, $temp;
	}
    }
    for (my $x=0;$x < $file;$x++) {
	my $temp = shift @mod;
	push @hold, $temp;
    }
    $current = shift @mod;
    if ($check == 1){
	stop();
    }
    else {
	play($current);	 
    }
}

sub repeat {
    my $test = "@_";
    if ($test == 0){
	$repeat = 1;
	return "Repeat ON";
    }
    else {
	$repeat = 0;
	return "Repeat OFF";
    }
}

sub random {
    my $test = "@_";
    if ($test == 0){
	$random = 1;
	return "Random ON";
    }
    else {
	$random = 0;
	return "Random OFF";
    }
}

sub lmod {
    print"Enter module: ";
    chomp(my $file = <STDIN>);
    if (-f $file){
	clist();
	$current = openf($file);
	if ($check == 1){
	    stop();
	}
	else {
	    play($current);	 
	}
	return "Loaded module: $current";
    }
    else {
	return "Can't open module.";
    }
}

sub opdir {
    print"Enter directory: ";
    chomp(my $dir = <STDIN>);
    if (-d $dir){
	opendir(DIR, "$dir");
	my @files = readdir DIR;
	closedir DIR;
	foreach my $entry (@files){
	    my $temp = "$dir/$entry";
	    if (-f $temp){
	    my $t2 = openf($temp);
	    if ($t2 ne "0") {
		push @mod, "$t2";
	    }
	}
	}
	smod();
	return "Enqueued directory: $dir";
    }
    else {
	return "Can't open directory.";
    }
}

sub openfile {
    print"Enter playlist to open: ";
    chomp(my $file = <STDIN>);
    if (-f $file){
	clist();
	if (defined $mod[0]) {
	    pop @mod;
	}
	open(PLAYFILE, "$file") or die "Can't open playlist: $!\n";
	foreach (<PLAYFILE>) {
	    chomp $_;
	    if (!-f $_) {
		next;
	    }
		push @mod, "$_";
	    }
	close PLAYFILE;
	return "Loaded playlist: $file";
    }
    else {
	return "Can't open playlist.";
    }
}

sub savefile {
    print"Enter name to save as: ";
    chomp(my $file = <STDIN>);
    my $hrm = savepl($file);
    return "$hrm";
}

sub opendefpl {
    if (-e "$ENV{HOME}/.wrap.pl"){
	open(PLAYFILE, "$ENV{HOME}/.wrap.pl") or die "Can't open playfile: $!\n";
	foreach (<PLAYFILE>) {
    	    chomp $_;
	    if (!-f $_) {
		next;
	    }
	    if ($_ eq "") {
		next;
	    }
	    push @mod, $_;
	}
	close PLAYFILE;
	smod();
    }
}


sub randmod {
    umod();
    my $roll = int (rand ($#mod+1));
    if ($roll !=  0){
	$roll = --$roll;
	my $c = splice(@mod,$roll,1);
	unshift @mod ,$c;
    }
}
sub autoskip {
    if ($check == 0) {
	play($current);
    }
    elsif ($check == 1) {
	$sub = 0;
	$check = 0;
	my $y = $#hold;
	if ($repeat == 1 && !defined $mod[0]) {
	    push @mod, $current;
	    for (my $x=0;$x <= $y;$x++) {
		my $temp = pop @hold;
		unshift @mod, $temp;
	    }
	    $current = shift @mod;
	    play($current);
	    return;
	}
	if (defined $mod[0]) {
	    smod();
	    play($current);
	}
    }
    elsif ($check == 5) {
	$check = 0;
    }
}


sub quit {
    print "QUIT\n";
    savepl("$ENV{HOME}/.wrap.pl");
    writecfg("$uade/.pwraprc");
    print $cls;
    print "QUIT\n";
    kill(9, -$$);
}

sub writecfg {
    my $file = "@_";
    if (-f $file) {
	return;
    }
    else {
	open (CONFIG , ">$file") or die "Can't create config file: $!\n";
	print CONFIG "#Shared uade directory\n";
	print CONFIG "UADE_SHARE_DIR:$uade\n";
	print CONFIG "\n";
	print CONFIG "#Directory where the uade binary is stored.\n";
	print CONFIG "UADE_BIN_DIR:$bindir\n";
	print CONFIG "\n";
	print CONFIG "#Default module dir when using the TK interface\n";
	print CONFIG "MODULE_DIR:$mymods\n";
	print CONFIG "\n";
	print CONFIG "#Local dir where pwrap will store downloaded modules\n";
	print CONFIG "FTP_LOCAL_DIR:$ftpldir\n";
	print CONFIG "\n";
	print CONFIG "#Aminet site we wanna connect to\n";
	print CONFIG "SITE:$site\n";
	print CONFIG "\n";
	print CONFIG "#Remote mod dir where the INDEX file is located\n";
	print CONFIG "FTP_REMOTE_DIR:$ftprdir\n";
	print CONFIG "\n";
	print CONFIG "#Username to site\n";
	print CONFIG "USER:$user\n";
	print CONFIG "\n";
	print CONFIG "#Password to site\n";
	print CONFIG "PASS:$pass\n";
	print CONFIG "\n";
	print CONFIG "#Use user\@host proxy. [TRUE/FALSE]\n";
	print CONFIG "PROXY:$proxy\n";
	print CONFIG "\n";
	print CONFIG "#If above is TRUE set proxy host here\n";
	print CONFIG "FIREWALL:$firewall\n";
	close CONFIG;
    }
}
sub stop {
    my $cp = "@_";
    local $SIG{QUIT} = 'IGNORE';
    if ($cp eq "5") {
	$check = 5;
	kill(3, -$$);
    }
    if ($check == 1){
	$check = 0;
	kill(3, -$$);
    }
    return "Stopping...";
}

sub nextmod {
    if (defined $mod[0]){
	$sub = 0;
	if ($check == 1){
	    smod();
	    stop();
	}
	else {
	    smod();
	}
	return "Skipping to the next module";
    }
    else {
	return "You are already on the last module";
    }
}

sub formtest {
    my $replayer=0;
    if (!defined $_[0]) {
	return "0";
    }
    my $file ="@_";
    $file = parsef($file);
    $file =~ m/(.*?)\.(.*?$)/;
    my $c=0;
    my $form = lc $1;
    $file =~ m/.*\.(.*?$)/;
    my $module = lc $1;
    foreach my $tst (@ext) {
	if ($tst eq $form) {
	    $replayer = $player[$c];
	}
	if ($tst eq $module) {
	    $replayer = $player[$c];
	}
	$c++;
    }
    if ($module eq "lzh" || $module eq "rar" || $module eq "lha" || $module eq "zip" || $module eq "bz2" || $module eq "gz" || $module eq "pp20" || $module eq "mmcp" || $module eq "xpk" || $module eq "sc68") {
        $replayer = "packed";
    }
    if (!defined $current) {
	return "0";
    }
    if ($replayer eq "0") {
	if (defined $mod[0]) {
	    $current = shift @mod;
	}
	else {
	    $current = pop @hold;
	}
	    $replayer = formtest($current);

    }
    return $replayer;
}

sub play {
    if ($check == 1 || !defined $_[0] ) {
	return "Nothing to play?";
    }
    $current = "@_";
    if (!-f $current) {
	$current = shift @mod;
	play($current);
    }
    $check = 1;
    if ($random == 1) {
	randmod();
	smod();
    }
    my $panning = ($pan/100);
    my $replayer = formtest($current);
    if ($replayer eq "0") {
	return "Nothing to play";
    }
    if ($sub != 0) {
	if ($replayer eq "packed"){
	    launch("$bindir/uade \"$current\" -sub $sub -pan $panning 1> /dev/null 2> /dev/null");
	}
	elsif ($replayer eq "custom"){
	    launch("$bindir/uade -P \"$current\" -sub $sub -pan $panning 1> /dev/null 2> /dev/null");
	}
	else {
	    launch("$bindir/uade -P $uade/players/$replayer -M \"$current\" -sub $sub -pan $panning 1> /dev/null 2> /dev/null");
	}	       		
    } 
    else {
	if ($replayer eq "packed"){
	    launch("$bindir/uade \"$current\" -pan $panning 1> /dev/null 2> /dev/null");
	}
	elsif ($replayer eq "custom"){
	    launch("$bindir/uade -P \"$current\" -pan $panning 1> /dev/null 2> /dev/null");
	}
	else {
	    launch("$bindir/uade -P $uade/players/$replayer -M \"$current\" -pan $panning 1> /dev/null 2> /dev/null");
	}	       		
    }
    return "Playing.....";
}

sub launch {
    my $pid;
    my $prog = "@_";
    if (defined $ARGV[0] && $ARGV[0] eq "--tk"){
	uptk();
    }
    else {
	drawmenu();
    }
    if ($pid = fork){
	return $pid;
    }
    elsif (defined $pid) {
	exec "$prog" or die "Can't start uade: $!\n";
	exit 0;
    }
    exit 0;
}

sub parsef {
    if (defined $_[0]) {
	my $modu = "@_";
	$modu =~ s/.*\///;
	return "$modu";
    }
}

sub openf {
    my $in = "@_";
    chomp $in;
    if (-d $in) {
	return"0";
    }
    my $cdir = getcwd();
    $in =~ s/(.*\/)//;
    my $dir = $1;
    if ($in eq "." or $in eq ".."){
	return"0";
    }
    if ($in =~ /^\..*/) {
	return"0";
    }
    if (!defined $dir) {
	return "$cdir/$in";
    } 
    else {
	chdir $dir;
	$dir = getcwd();
	chdir $cdir;
	return "$dir/$in";
    }
}

sub usage {
    print "\nUSAGE: <program> [switch] (module1) (module2) etc.\n\n";
    print " --help            for this help\n";
    print " --pl playlist     to load a playlist on startup\n";
    print " --tk              To start the player with a perl/tk gui.\n";
    print "                   You'll need perl/tk installed for this.\n";
    print "                   Perl/tk can be found at the link below.\n";
    print "                   http://perl.com/CPAN/modules/by-authors/Nick_Ing-Simmons/\n\n";
    print "---\n";
    print "All modules specified on the commandline ends up in a playfile.\n";
    print "Wildcards on the command line works too ie. *.mod etc.\n";
    print "You can't specify both modules and switches.\n\n";
    print "If the program is started without any arguments it will\n";
    print "try to open a default playlist in \$HOME/.wrap.pl.\n\n";
    print "This playlist is created by saving everything in your\n";
    print "playlist when you quit the program.\n\n";
    print "After this the menu will appear.\n";
    print "---\n\n";
    exit;
}

BEGIN {
use POSIX qw(:termios_h);
my ($term, $oterm, $echo, $noecho, $fd_stdin);
$fd_stdin = fileno(STDIN);
$term = POSIX::Termios->new();
$term->getattr($fd_stdin);
$oterm = $term->getlflag();
$echo = ECHO|ECHOK|ICANON;
$noecho = $oterm & ~$echo;
sub cbreak {
    $term->setlflag($noecho);
    $term->setcc(VTIME, 1);
    $term->setattr($fd_stdin, TCSANOW);
}
sub cooked {
    $term->setlflag($oterm);
    $term->setcc(VTIME,0);
    $term->setattr($fd_stdin, TCSANOW);
}
sub getone {
    my $key = "";
    cbreak();
    sysread(STDIN, $key, 1);
    cooked();
    return $key;
}
}
END { cooked() }

sub back {
    if (defined $hold[0]){
	$sub = 0;
	if ($check == 1){
	    umod();
	    stop();
	}
	else {
	    umod();
	}
	return "Going back to the previous module.";
    }
    else {
	return "You are already on the first module";
    }
}

sub remcur {
    if (defined $mod[0]){
	my $shit = parsef($current);
	$sub = 0;
	$current = shift @mod;
	if ($check == 1){
	    stop();
	}
	else {
	    play($current);	 
	}
	return "Removed $shit from playlist";
    }
    else {
	return "You are not allowed to remove the last file";
    }
}

sub smod {
    if (defined $mod[0]) {
	if (defined $current) {
	    push @hold, $current;
	}
	$current = shift @mod;
    }
}

sub umod {
    unshift @mod, $current;
    $current = pop @hold;
}

sub nsub {
    $sub++;
    stop();
    return "Skipping to next subsong";
}

sub psub {
    if ($sub != 0){
	$sub--;
	stop();
	return "Rewinding to previous subsong";
    }
    else {
	return "You are already playing the first subsong";
    }
}

sub readformats {
    open(FORMATS, "$uade/players/uadeformats") or die "Can't find formats file: $!\n";
    my $x=0;
    foreach (<FORMATS>) {
	chomp;
	m/(.*?)\s(.*?$)/;
      	$player[$x] = $2;
        $ext[$x] = $1;
	$x++;
    }
close FORMATS;

if (-f "$uade/.pwraprc") {
    open (CONFIG, "$uade/.pwraprc") or die "Can't open config file: $!\n";  
    foreach (<CONFIG>) {
	chomp;
	if (m/UADE_SHARE_DIR:(.*)/) {
	    $uade = "$1";
	}
	if (m/UADE_BIN_DIR:(.*)/) {
	    $bindir = "$1";
	}
	if (m/MODULE_DIR:(.*)/) {
	    $mymods = "$1";
	}
	if (m/FTP_LOCAL_DIR:(.*)/) {
	    $ftpldir = "$1";
	}
	if (m/FTP_REMOTE_DIR:(.*)/) {
	    $ftprdir = "$1";
	}
	if (m/SITE:(.*)/) {
	    $site = "$1";
	}
	if (m/USER:(.*)/) {
	    $user = "$1";
	}
	if (m/PASS:(.*)/) {
	    $pass = "$1";
	}
	if (m/PROXY:(.*)/) {
	    $proxy = "$1";
	}
	if (m/FIREWALL:(.*)/) {
	    $firewall = "$1";
	}
    }
    close CONFIG;
    }
}

sub clist {
    if (defined $mod[0]) {
	for (my $x=0;$x <= $#mod;) {
	    pop @mod;
	}
    }
    if (defined $hold[0]){
	for (my $x=0;$x <= $#hold;) {
	    pop @hold;
	}
    }
    return "Playlist cleared";
}

sub savepl {
    my $savefile = "@_";
    if (-d $savefile){
	return "Couldn't save playlist.";
    }
    elsif (-e $savefile && -w $savefile != 1) {
	return "Couldn't save playlist.";
    } 
    else {
	open(SAVEFILE, ">$savefile") or die "Can't save playlist: $!\n";
	if (defined $hold[0]) {
	    foreach my $entry (@hold) {
		chomp $entry;
		print SAVEFILE "$entry\n";
	    }
	}
	if (defined $current) {
	    print SAVEFILE "$current\n";
	}
	if (defined $mod[0]) {
	    foreach my $entry (@mod) {
		chomp $entry;
		print SAVEFILE "$entry\n";
	    }
	}
	close SAVEFILE;
	return "Saved playlist as: $savefile";
    }
}

sub dispfile {
    $i=0;
    print $cls;
    my $c=0;	
    foreach (@hold){
	$c++;
	formprint($c,(parsef($_)));
	my $p = pause();
	if ($p eq "1") {
	    return "Displayed current playlist";
	}
    }
    $c++;
    formprint("PLAY:",(parsef($current)));
    foreach (@mod) {
	$c++;
	formprint($c,(parsef($_)));
	my $p = pause();
	if ($p eq "1") {
	    return "Displayed current playlist";
	}
    }
    print"---Press any key to continue---\n";
    getone();    
    return "Displayed current playlist";
}

sub formprint {
    my ($c,$d) = @_;
    format ftpwrite =
@<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$c, $d
.
    local $~ = "ftpwrite";
    write;
}

sub pause {
    $i++;
    if ($i == 21) {
	print"---Press any key to continue or q to exit---\n";
	my $t = getone();
	if ($t eq "q") {
	    $i=0;
	    return "1";
	}
	else {
	    $i=0
	    }
    }
    else {
	return "0";
    }
}

sub getindex{
    local $SIG{QUIT} = 'IGNORE';
    my $indexcmd = @_;
    
    if ($indexcmd eq "1") {
	unlink "$ftpldir/INDEX";
    }
    
    if (!-f "$ftpldir/INDEX") {
	print "No INDEX file found. Downloading it...\n";
	print "Config file used is $uade/.pwraprc\n";
	print "There you can change which aminet site to use among other things\n";
	if ($proxy eq "TRUE") {
	    $ftp = Net::FTP->new("$site", Firewall => "$firewall", Debug => 0);
	}
	else {
	    $ftp = Net::FTP->new("$site", Debug => 0);
	}
	$ftp->login("$user",'$pass');
	$ftp->cwd("$ftprdir");
	$ftp->get("$ftprdir/INDEX", "$ftpldir/INDEX");
	$ftp->quit;
    }
    if (!defined $index[0]){
	if (-f "$ftpldir/INDEX"){
	    open(INDEX1, "$ftpldir/INDEX") or die "Can't open INDEX file: $!\n";
	    foreach (<INDEX1>) {
		chomp;
		push @index, $_;
	    }
	    shift @index;
	    shift @index;
	    close INDEX1;
	}
    	return ("Index Downloaded");
    }
}

sub ijump {
    getindex();
    $i=0;
    print"Enter mod number to download, search string or list: ";
    chomp(my $numb = <STDIN>);
    if ($numb eq "") {
	if (defined $ftp) {
	    $ftp->quit;
	}
	return("Done");
    }
    if ($numb eq "list") {
	my $c=0;
	if (defined $index[0]){
	    foreach (@index){
		$c++;
		formprint($c, $_);
		my $p = pause();
		if ($p eq "1") {
		    last;
		}
	    }
	}
	ijump();
    }
    elsif ($numb =~ /[^0-9:]/) {
	print $cls;
	my $c=0;	
	if (defined $index[0]){
	    foreach my $temp (@index){
		$c++;
#		my $temp = parsei($_);
		if ($temp =~ /.*$numb.*/i) {
		    $temp = $index[($c-1)];
		    formprint($c,$temp);
		    my $p = pause();
		    if ($p eq "1") {
			last;
		    }
		}
	    }
	}
	ijump();    
    }
    else {
	$numb--;
	my $msg = geti($numb);
	return($msg);
    }
}

sub fcheck {
    my $file = "@_";
    if (-f "$file") {
	print "$1 has already been downloaded. Do you want to overwrite [N/y]:\n ";
	my $q = lc getone();
	if ($q eq "y") {
	    return("1");
	}
	else {
	    return ("0");
	}
    }
    else {
	return ("2");
    }
}

sub geti {
    my $mod ="@_";
    my $pid;
    my $temp = $index[$mod];
    $temp =~ /(^.*?)[ ]+mods\/(.*?) /;
    my $file = "$ftpldir/$1";
    my $q = fcheck($file);
	if ($q eq "1") {
	    if ($pid = fork){
		push @pids, $pid;
		$down++;
		return("Downloading $1");
	    }
	    elsif (defined $pid) {
		local $SIG{QUIT} = 'IGNORE';
		if ($proxy eq "TRUE") {
		    $ftp = Net::FTP->new("$site", Firewall => "$firewall", Debug => 0);
		}
		else {
		    $ftp = Net::FTP->new("$site", Debug => 0);
		}
		$ftp->login("$user",'$pass');
		$ftp->cwd("$ftprdir");
		$ftp->get("$ftprdir/$2/$1", "$file");    
		tfile("$file");
		$ftp->quit;
		_exit(0);
	    }
	}
	if ($q eq "0") {
	    qfile("$file");
	    return("Adding $1 to playlist");
	}
    if ($q eq "2") {
	if ($pid = fork){
	    push @pids, $pid;
	    $down++;
	    return("Downloading $1");
	}
	elsif (defined $pid) {
	    local $SIG{QUIT} = 'IGNORE';
	    if ($proxy eq "TRUE") {
		$ftp = Net::FTP->new("$site", Firewall => "$firewall", Debug => 0);
	    }
	    else {
		$ftp = Net::FTP->new("$site", Debug => 0);
	    }
	    $ftp->login("$user",'$pass');
	    $ftp->cwd("$ftprdir");
	    $ftp->get("$ftprdir/$2/$1", "$file");    
	    tfile("$file");
	    $ftp->quit;
	    _exit(0);
	}
    }
}

sub parsei {
    my $modu = "@_";
    $modu =~ m/(.*?) /i;
    $modu = $1;
    return "$modu";
}

sub qfile {
    my $file = "@_";
    if (-f $file){
	my $t2 = openf($file);
	if ($t2 ne "0") {
	    push @mod, "$t2";
	}
    }
    return;
}

sub tfile {
    my $file = "@_";
    open(TEMPFILE, ">/tmp/.pwraptemp") or die "Can't save tempfile: $!\n";
    print TEMPFILE "$file\n";
    close TEMPFILE;
}

sub rfile {
    my $file;
    open(TEMPFILE, "/tmp/.pwraptemp") or die "Can't open tempfile: $!\n";
	foreach (<TEMPFILE>) {
	    chomp $_;
	    $file = openf($_);
	    push @mod, $file;
	}
        close TEMPFILE;
    return($file);
}
