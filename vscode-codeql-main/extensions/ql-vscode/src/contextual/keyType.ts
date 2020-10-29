export enum KeyType {
  DefinitionQuery = 'DefinitionQuery',
  ReferenceQuery = 'ReferenceQuery',
  PrintAstQuery = 'PrintAstQuery',
}

export function tagOfKeyType(keyType: KeyType): string {
  switch (keyType) {
    case KeyType.DefinitionQuery:
      return 'ide-contextual-queries/local-definitions';
    case KeyType.ReferenceQuery:
      return 'ide-contextual-queries/local-references';
    case KeyType.PrintAstQuery:
      return 'ide-contextual-queries/print-ast';
  }
}

export function nameOfKeyType(keyType: KeyType): string {
  switch (keyType) {
    case KeyType.DefinitionQuery:
      return 'definitions';
    case KeyType.ReferenceQuery:
      return 'references';
    case KeyType.PrintAstQuery:
      return 'print AST';
  }
}

export function kindOfKeyType(keyType: KeyType): string {
  switch (keyType) {
    case KeyType.DefinitionQuery:
    case KeyType.ReferenceQuery:
      return 'definitions';
    case KeyType.PrintAstQuery:
      return 'graph';
  }
}
