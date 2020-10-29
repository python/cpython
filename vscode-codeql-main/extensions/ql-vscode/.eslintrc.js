module.exports = {
  parser: "@typescript-eslint/parser",
  parserOptions: {
    ecmaVersion: 2018,
    sourceType: "module",
    project: ["tsconfig.json", "./src/**/tsconfig.json", "./gulpfile.ts/tsconfig.json"],
  },
  plugins: ["@typescript-eslint"],
  env: {
    node: true,
    es6: true,
  },
  extends: ["eslint:recommended", "plugin:@typescript-eslint/recommended"],
  rules: {
    "@typescript-eslint/no-use-before-define": 0,
    "@typescript-eslint/no-unused-vars": [
      "warn",
      {
        vars: "all",
        args: "none",
        ignoreRestSiblings: false,
      },
    ],
    "@typescript-eslint/explicit-function-return-type": "off",
    "@typescript-eslint/no-non-null-assertion": "off",
    "@typescript-eslint/no-explicit-any": "off",
    "prefer-const": ["warn", { destructuring: "all" }],
    indent: "off",
    "@typescript-eslint/indent": "off",
    "@typescript-eslint/no-throw-literal": "error",
    "no-useless-escape": 0,
    semi: 2,
    quotes: ["warn", "single"]
  },
};
