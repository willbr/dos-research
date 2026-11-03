Glide 2.43 Rasterization Library (SDK)
for Voodoo Graphics and Voodoo Rush

This document contains Runtime Driver notes.
Please see the Glide Release Notes for detailed information
on using this version of the Glide Rasterization Library.

Platform: Windows 95 and Windows NT

Copyright ( 1997 3Dfx Interactive, Inc. )
All Rights Reserved

3Dfx Interactive, Inc.
website: www.3dfx.com
news server: news.3dfx.com


=======================================================================
What's in the distribution?
=======================================================================
This SDK distribution contains the Glide 2.43 drivers for Voodoo 
Graphics Based 3D accelerators and Voodoo Rush based 3D accelerators.

The following Voodoo Graphics will function with these drivers:

- 3Dfx Interactive reference boards
- Diamond Monster 3D
- Orchid Righteous 3D
- Deltron Realvision Flash 3D
- Skywell Technology Magic 3D
- Guillemot MaxiGamer
- Canopus Pure 3D
- Miro HiScore

The following Voodoo Rush boards function with these drivers:

- Hercules Stingray 128-3D
- Jazz Multimedia Adrenaline Rush 3D
- Intergraph Intense 3D Voodoo


NOTE:  End-users are recommended to use drivers provided by their
board manufacturer to ensure that they can obtain full support.

Included Runtime Driver Files
-----------------------------

Voodoo Graphics - Windows 95 and DOS:

GLIDE2X.DLL  - Glide Voodoo Graphics Driver for Windows 95
GLIDE2X.OVL  - Glide Voodoo Graphics Driver for DOS


Voodoo Rush - Windows 95 and DOS:

GLIDE2X.DLL  - Glide Voodoo Graphics Driver for Windows 95
GLIDE2X.OVL  - Glide Voodoo Graphics Driver for DOS


Voodoo Graphics - Windows NT:

GLIDE2X.DLL  - Glide Voodoo Graphics Driver for Windows NT
GENPORT.SYS  - Windows NT Device Driver for Voodoo Graphics
MAPMEM.SYS   - Windows NT Device Driver for Voodoo Graphics
NTREMAP.SYS  - Windows NT Device Driver for Voodoo Graphics


General Files:

README.TXT   - This File
FXMEMMAP.VXD - Windows 95 VXD for 3Dfx based hardware

=======================================================================
Installation 
=======================================================================

Requirements
------------

- Windows 95 (Supported by Voodoo Graphics and Voodoo Rush drivers)
- Windows NT 4.0 (Supported by Voodoo Graphics drivers only)
- PC with a Pentium or Pentium Pro Processor and a free PCI slot

Installation
------------

NOTE: If the setup program is run under Windows 95, both Windows
and DOS drivers are installed.  

If the setup program is run under Windows NT (while using an 
Administrator account) Windows NT drivers and 3Dfx device services
are installed. (Currently, only Voodoo Graphics is supported under
Windows NT)

COMPATABILITY NOTICE: If you are using 3Dfx Direct3D drivers for 
Voodoo Graphics with the Glide 2.43 runtime drivers, you MUST upgrade
to 3Dfx Direct3D 2.1 or higher drivers.  The FXMEMMAP.VXD included in 
this kit is NOT compatible with the version provided with older 3Dfx
Direct3D drivers.  If you do NOT use the 3Dfx Direct3D drivers you
don't need to do anything.

1) Run the Glide 2.43 SDK Setup Program 
2) Follow the on screen prompts.
3) When prompted for Runtime Driver Installation if the correct
   hardware is selected, click Next.
4) After installation you may need to reboot the system.



Troubleshooting Voodoo Graphics
-------------------------------

####

Q: I have an S3 968/868 video card, why does my system lock
up when I run a Glide or Direct3D title.

A: S3 968 and 868 chipsets may misreport the mapped memory used.
There is a chance that your Voodoo Graphics card will be 
assigned by Plug and Play a memory range that is used by the S3
card, but not reported.   The Voodoo Graphics card can be 
relocated by:


	Windows 95 Users:  Please download the S3 Fix kit
	from the Downloads/Updates section of http://www.3dfx.com


	Windows NT Users: An NT service called NTREMAP is 
        installed when Glide is added to an NT system.
	NTREMAP can change the memory address of a Voodoo 
	Graphics board and prevent a conflict with an S3 card.

	To use NTREMAP, add the following registry values to:
	\HKLM\Software\3Dfx Interactive\NTRemap
	
	Value Name	Type	Value
	----------      ----    -----
	EnableNTRemap	DWORD	1 (HEX) 
	ForceAddress	DWORD	12000000 (HEX)

	The Force Address setting will vary, in this example
	0x12000000 is used.  This is normally free on most
	systems.

	NOTE: If you use an address that over-writes another
	device, Windows NT may not boot.  If such a problem 
	occurs, power off the system and remove your Voodoo 
	Graphics.  Alter the registry to use a different
	memory range (or remove the ForceAddress setting).
	Then Reinstall the Voodoo Graphics card into your 
	system.


####

Q: The display seems dark when a Glide title is running, how
do I make the display brighter?

A: Using a file editor (ie: Notepad), edit your \AUTOEXEC.BAT
file.  Add the following lines

	SET SST_RGAMMA=1.7
	SET SST_BGAMMA=1.7
	SET SST_GGAMMA=1.7

	1.7 is the default value, increasing past this setting
	will raise gamma.  It is however suggested that you
	don't raise the setting past 1.9

####



Troubleshooting Voodoo Rush
-------------------------------

####

Q: The display seems dark when a Windows based Glide title is
running, how do I make the display brighter?

A: Adjust the Video Chroma setting by clicking on the taskbar
icon.  This will brighten Windows based Glide titles.  It will
not adjust DOS based Glide titles.

####
