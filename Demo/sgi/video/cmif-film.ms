.de PP
.LP
..
.de pT
.IP \fB\\$1\fP
..
.TL
CMIF video file format
.AU
Jack Jansen
(Version of 27-Feb-92)
.SH
Introduction
.PP
The CMIF video format was invented to allow various applications
to exchange video data. The format consists of
a header containing global information (like data format)
followed by a sequence of frames, each consisting of a header
followed by the actual frame data.
All information except pixel data is
encoded in ASCII. Pixel data is \fIalways\fP encoded in Silicon Graphics
order, which means that the first pixel in the frame is the lower left
pixel on the screen.
.PP
All ASCII data except the first line of the file
is in python format. This means that
outer parentheses can be ommitted, and parentheses around a tuple with
one element can also be omitted. So, the lines
.IP
.ft C
.nf
('grey',(4))
('grey',4)
'grey',4
.LP
have the same meaning. 
To ease parsing in C programs, however, it is advised that there are
no parenteses around single items, and that there are parentheses around
lists. So, the second format above is preferred.
.PP
The current version is version 3, but this document will also explain
shortly what the previous formats looked like.
.SH
Header.
.PP
The header consists of three lines. The first line identifies the file
as a CMIF video file, and gives the version number.
It looks as follows:
.IP
.ft C
CMIF video 3.0
.LP
All programs expect the layout to be exactly like this, so no
extra spaces, etc. should be added.
.PP
The second line specifies the data format. Its format is a python
tuple with two members. The first member is a string giving the format
type and the second is a tuple containing type-specific information.
The following formats are currently understood:
.pT rgb
The video data is 24 bit RGB packed into 32 bit words. 
R is the least significant byte, then G and then B. The top byte is
unused.
.IP
There is no type-specific information, so the complete data format
line is
.IP
.ft C
('rgb',())
.pT grey
The video data is greyscale, at most 8 bits. Data is packed into
8 bit bytes (in the low-order bits). The extra information is the
number of significant bits, so an example data format line is
.IP
.ft C
('grey',(6))
.pT yiq
The video data is in YIQ format. This is a format that has one luminance
component, Y, and two chrominance components, I and Q. The luminance and
chrominance components are encoded in \fItwo\fP pixel arrays: first an
array of 8-bit luminance values followed by a array of 16 bit chrominance
values. See the section on chrominance coding for details.
.IP
The type specific part contains the number of bits for Y, I and Q,
the chrominance packfactor and the colormap offset. So, a sample format
information line of
.IP
.ft C
('yiq',(5,3,3,2,1024))
.IP
means that the pictures have 5 bit Y values (in the luminance array),
3 bits of I and Q each (in the chrominance array), chrominance data
is packed for 2x2 pixels, and the first colormap index used is 1024.
.pT hls
The video data is in HLS format. L is the luminance component, H and S
are the chrominance components. The data format and type specific information
are the same as for the yiq format.
.pT hsv
The video data is in HSV format. V is the luminance component, H and S
are the chrominance components. Again, data format and type specific
information are the same as for the yiq format.
.pT rgb8
The video data is in 8 bit dithered rgb format. This is the format
used internally by the Indigo. bit 0-2 are green, bit 3-4 are blue and
bit 5-7 are red. Because rgb8 is treated more-or-less like yiq format
internally the type-specific information is the same, with zeroes for
the (unused) chrominance sizes:
.IP
.ft C
('rgb8',(8,0,0,0,0))
.PP
The third header line contains width and height of the video image,
in pixels, and the pack factor of the picture. For compatability, RGB
images must have a pack factor of 0 (zero), and non-RGB images must
have a pack factor of at least 1.
The packfactor is the amount of compression done on the original video
signal to obtain pictures. In other words, if only one out of three pixels
and lines is stored (so every 9 original pixels have one pixel in the
data) the packfactor is three. Width and height are the size of the
\fIoriginal\fP picture.
Viewers are expected to enlarge the picture so it is shown in the
original size. RGB videos cannot be packed.
So, a size line like
.IP
.ft C
200,200,2
.LP
means that this was a 200x200 picture that is stored as 100x100 pixels.
.SH
Frame header
.PP
Each frame is preceded by a single header line. This line contains timing information
and optional size information. The time information is mandatory, and
contains the time this frame should be displayed, in milliseconds since
the start of the film. Frames should be stored in chronological order.
.PP
An optional second number is interpreted as the size of the luminance
data in bytes. Currently this number, if present, should always be the
same as \fCwidth*height/(packfactor*packfactor)\fP (times 4 for RGB
data), but this might change if we come up with variable-length encoding
for frame data.
.PP
An optional third number is the size of the chrominance data
in bytes. If present, the number should be equal to
.ft C
luminance_size2*/(chrompack*chrompack).
.SH
Frame data
.PP
For RGB films, the frame data is an array of 32 bit pixels containing
RGB data in the lower 24 bits. For greyscale films, the frame data
is an array of 8 bit pixels. For split luminance/chrominance films the
data consists of two parts: first an array of 8 bit luminance values
followed by an  array of 16 bit chrominance values.
.PP
For all data formats, the data is stored left-to-right, bottom-to-top.
.SH
Chrominance coding
.PP
Since the human eye is apparently more sensitive to luminance changes
than to chrominance changes we support a coding where we split the luminance
and chrominance components of the video image. The main point of this
is that it allows us to transmit chrominance data in a coarser granularity
than luminance data, for instance one chrominance pixel for every
2x2 luminance pixels. According to the theory this should result in an
acceptable picture while reducing the data by a fair amount.
.PP
The coding of split chrominance/luminance data is a bit tricky, to
make maximum use of the graphics hardware on the Personal Iris. Therefore,
there are the following constraints on the number of bits used:
.IP -
No more than 8 luminance bits,
.IP -
No more than 11 bits total,
.IP -
The luminance bits are in the low-end of the data word, and are stored
as 8 bit bytes,
.IP -
The two sets of chrominance bits are stored in 16 bit words, correctly
aligned,
.IP -
The color map offset is added to the chrominance data. The offset should
be at most 4096-256-2**(total number of bits). To reduce interference with
other applications the offset should be at least 1024.
.LP
So, as an example, an HLS video with 5 bits L, 4 bits H, 2 bits S and an
offset of 1024 will look as follows in-core and in-file:
.IP
.nf
.ft C
	  31         15    11 10 9 8  5 4   0
         +-----------------------------------+
incore   +               0+ 1+  S +  H +   L +
         +-----------------------------------+
                                  +----------+
L-array                           +  0 +   L +
                                  +----------+
                     +-----------------------+
C-array              +   0+ 1+  S +  H +   0 +
                     +-----------------------+
