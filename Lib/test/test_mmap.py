
import mmap
import string, os, re, sys

PAGESIZE = mmap.PAGESIZE

def test_unix():
    "Test mmap module on Unix systems"
    	
    # Create an mmap'ed file
    f = open('foo', 'w+')
    
    # Write 2 pages worth of data to the file
    f.write('\0'* PAGESIZE)
    f.write('foo')
    f.write('\0'* (PAGESIZE-3) )
    
    m = mmap.mmap(f.fileno(), 2 * PAGESIZE)
    f.close()
    
    # Simple sanity checks
    print '  Position of foo:', string.find(m, 'foo') / float(PAGESIZE), 'pages'
    assert string.find(m, 'foo') == PAGESIZE
    
    print '  Length of file:', len(m) / float(PAGESIZE), 'pages'
    assert len(m) == 2*PAGESIZE

    print '  Contents of byte 0:', repr(m[0])
    assert m[0] == '\0'
    print '  Contents of first 3 bytes:', repr(m[0:3])
    assert m[0:3] == '\0\0\0'
    
    # Modify the file's content
    print "\n  Modifying file's content..."
    m[0] = '3'
    m[PAGESIZE +3: PAGESIZE +3+3]='bar'
    
    # Check that the modification worked
    print '  Contents of byte 0:', repr(m[0])
    assert m[0] == '3'
    print '  Contents of first 3 bytes:', repr(m[0:3])
    assert m[0:3] == '3\0\0'
    print '  Contents of second page:',  m[PAGESIZE-1 : PAGESIZE + 7]
    assert m[PAGESIZE-1 : PAGESIZE + 7] == '\0foobar\0'
    
    m.flush()

    # Test doing a regular expression match in an mmap'ed file
    match=re.search('[A-Za-z]+', m)
    if match == None:
        print '  ERROR: regex match on mmap failed!'
    else:
        start, end = match.span(0)
        length = end - start               

        print '  Regex match on mmap (page start, length of match):',
        print start / float(PAGESIZE), length
        
        assert start == PAGESIZE
        assert end == PAGESIZE + 6
        
    print ' Test passed'

# XXX need to write a test suite for Windows
if sys.platform == 'win32':
    pass
else:
    test_unix()
    
