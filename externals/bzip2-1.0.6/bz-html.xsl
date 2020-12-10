<?xml version="1.0"?> <!-- -*- sgml -*- -->
<!DOCTYPE xsl:stylesheet [ <!ENTITY bz-css SYSTEM "./bzip.css"> ]>

<xsl:stylesheet 
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>
<xsl:import href="bz-common.xsl"/>

<!-- use 8859-1 encoding -->
<xsl:output method="html" encoding="ISO-8859-1" indent="yes"/>

<!-- we include the css directly when generating one large file -->
<xsl:template name="user.head.content">  
  <style type="text/css" media="screen">
    <xsl:text>&bz-css;</xsl:text>
  </style>
</xsl:template>

</xsl:stylesheet>
