import { languages, IndentAction, OnEnterRule } from 'vscode';

/**
 * OnEnterRules are available in language-configurations, but you cannot specify them in the language-configuration.json.
 * They can only be specified programmatically.
 *
 * Also, we should keep the language-configuration.json as a json file and register it in the package.json because
 * it is registered first, before the extension is activated, so language features are available quicker.
 *
 * See https://github.com/microsoft/vscode/issues/11514
 * See https://github.com/microsoft/vscode/blob/master/src/vs/editor/test/common/modes/supports/javascriptOnEnterRules.ts
 */
export function install() {
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const langConfig = require('../language-configuration.json');
  // setLanguageConfiguration requires a regexp for the wordpattern, not a string
  langConfig.wordPattern = new RegExp(langConfig.wordPattern);
  langConfig.onEnterRules = onEnterRules;
  langConfig.indentationRules = {
    decreaseIndentPattern: /^((?!.*?\/\*).*\*\/)?\s*[\}\]].*$/,
    increaseIndentPattern: /^((?!\/\/).)*(\{[^}"'`]*|\([^)"'`]*|\[[^\]"'`]*)$/
  };

  languages.setLanguageConfiguration('ql', langConfig);
  languages.setLanguageConfiguration('qll', langConfig);
  languages.setLanguageConfiguration('dbscheme', langConfig);
}

const onEnterRules: OnEnterRule[] = [
  {
    // e.g. /** | */
    beforeText: /^\s*\/\*\*(?!\/)([^\*]|\*(?!\/))*$/,
    afterText: /^\s*\*\/$/,
    action: { indentAction: IndentAction.IndentOutdent, appendText: ' * ' },
  },
  {
    // e.g. /** ...|
    beforeText: /^\s*\/\*\*(?!\/)([^\*]|\*(?!\/))*$/,
    action: { indentAction: IndentAction.None, appendText: ' * ' },
  },
  {
    // e.g.  * ...|
    beforeText: /^(\t|[ ])*[ ]\*([ ]([^\*]|\*(?!\/))*)?$/,
    // oneLineAboveText: /^(\s*(\/\*\*|\*)).*/,
    action: { indentAction: IndentAction.None, appendText: '* ' },
  },
  {
    // e.g.  */|
    beforeText: /^(\t|[ ])*[ ]\*\/\s*$/,
    action: { indentAction: IndentAction.None, removeText: 1 },
  },
  {
    // e.g.  *-----*/|
    beforeText: /^(\t|[ ])*[ ]\*[^/]*\*\/\s*$/,
    action: { indentAction: IndentAction.None, removeText: 1 },
  },
];
