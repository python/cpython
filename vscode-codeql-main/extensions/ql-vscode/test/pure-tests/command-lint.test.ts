import { expect } from 'chai';
import * as path from 'path';
import * as fs from 'fs-extra';

type CmdDecl = {
  command: string;
  when?: string;
  title?: string;
}

describe('commands declared in package.json', function() {
  const manifest = fs.readJsonSync(path.join(__dirname, '../../package.json'));
  const commands = manifest.contributes.commands;
  const menus = manifest.contributes.menus;

  const disabledInPalette: Set<string> = new Set<string>();

  // These commands should appear in the command palette, and so
  // should be prefixed with 'CodeQL: '.
  const paletteCmds: Set<string> = new Set<string>();

  // These commands arising on context menus in non-CodeQL controlled
  // panels, (e.g. file browser) and so should be prefixed with 'CodeQL: '.
  const contribContextMenuCmds: Set<string> = new Set<string>();

  // These are commands used in CodeQL controlled panels, and so don't need any prefixing in their title.
  const scopedCmds: Set<string> = new Set<string>();
  const commandTitles: { [cmd: string]: string } = {};

  commands.forEach((commandDecl: CmdDecl) => {
    const { command, title } = commandDecl;
    if (
      command.match(/^codeQL\./)
      || command.match(/^codeQLQueryResults\./)
      || command.match(/^codeQLTests\./)
    ) {
      paletteCmds.add(command);
      expect(title).not.to.be.undefined;
      commandTitles[command] = title!;
    }
    else if (
      command.match(/^codeQLDatabases\./)
      || command.match(/^codeQLQueryHistory\./)
      || command.match(/^codeQLAstViewer\./)
    ) {
      scopedCmds.add(command);
      expect(title).not.to.be.undefined;
      commandTitles[command] = title!;
    }
    else {
      expect.fail(`Unexpected command name ${command}`);
    }
  });

  menus['explorer/context'].forEach((commandDecl: CmdDecl) => {
    const { command } = commandDecl;
    paletteCmds.delete(command);
    contribContextMenuCmds.add(command);
  });

  menus['editor/context'].forEach((commandDecl: CmdDecl) => {
    const { command } = commandDecl;
    paletteCmds.delete(command);
    contribContextMenuCmds.add(command);
  });

  menus.commandPalette.forEach((commandDecl: CmdDecl) => {
    if (commandDecl.when === 'false')
      disabledInPalette.add(commandDecl.command);
  });



  it('should have commands appropriately prefixed', function() {
    paletteCmds.forEach(command => {
      expect(commandTitles[command], `command ${command} should be prefixed with 'CodeQL: ', since it is accessible from the command palette`).to.match(/^CodeQL: /);
    });

    contribContextMenuCmds.forEach(command => {
      expect(commandTitles[command], `command ${command} should be prefixed with 'CodeQL: ', since it is accessible from a context menu in a non-extension-controlled context`).to.match(/^CodeQL: /);
    });

    scopedCmds.forEach(command => {
      expect(commandTitles[command], `command ${command} should not be prefixed with 'CodeQL: ', since it is accessible from an extension-controlled context`).not.to.match(/^CodeQL: /);
    });
  });

  it('should have the right commands accessible from the command palette', function() {
    paletteCmds.forEach(command => {
      expect(disabledInPalette.has(command), `command ${command} should be enabled in the command palette`).to.be.false;
    });

    // Commands in contribContextMenuCmds may reasonbly be enabled or
    // disabled in the command palette; for example, codeQL.runQuery
    // is available there, since we heuristically figure out which
    // query to run, but codeQL.setCurrentDatabase is not.

    scopedCmds.forEach(command => {
      expect(disabledInPalette.has(command), `command ${command} should be disabled in the command palette`).to.be.true;
    });
  });


});
