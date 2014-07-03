#!/usr/bin/perl

my $num_args = $#ARGV + 1;
if ($num_args != 2) {
  print "\nUsage: create_grub2_entry_nova.perl builddir run_name\n";
  exit;
}

my $builddir=$ARGV[0];
my $run_name=$ARGV[1];

open FILE, "$builddir/var/run/$run_name/boot/grub/menu.lst" or die $!;
open GRUB, ">$builddir/var/run/$run_name/40_custom" or die $!;

print GRUB "#!/bin/sh
exec tail -n +3 \$0
# This file provides an easy way to add custom menu entries.  Simply type the
# menu entries you want to add after this comment.  Be careful not to change
# the 'exec tail' line above.

menuentry \"Genode NOVA\" {
 insmod ext2
 multiboot /hypervisor iommu serial nopcid
";

while (<FILE>) {

  if ($_ =~ m/genode/)
  {
    chomp;      
    print GRUB;
    my @line = split /\//;
   print GRUB " $line[2]\n";
  }

}

print GRUB "}";

close GRUB or die $!;
close FILE or die $!;

