<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://schemas.microsoft.com/wix/2006/localization">
    <xsl:output indent="yes"/>
    <xsl:strip-space elements="*"/>

    <xsl:template match="node()|@*">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
    </xsl:template>

    <xsl:template match="*[local-name()='String' and @Id='InstallButtonNote']">
        <String Id="InstallButtonNote">[TargetDir]

Includes IDLE, pip and documentation
Creates shortcuts but no file associations</String>
    </xsl:template>

    <xsl:template match="*[local-name()='String' and @Id='Include_launcherHelp']">
        <String Id="Include_launcherHelp">(The 'py' launcher is currently unavailable on ARM64.)</String>
    </xsl:template>
</xsl:stylesheet>