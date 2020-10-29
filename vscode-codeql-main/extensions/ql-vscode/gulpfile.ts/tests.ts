import * as gulp from 'gulp';

export function copyTestData() {
  return gulp.src('src/vscode-tests/no-workspace/data/**/*')
    .pipe(gulp.dest('out/vscode-tests/no-workspace/data'));
}
