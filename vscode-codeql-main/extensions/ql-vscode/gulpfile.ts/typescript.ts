import * as colors from 'ansi-colors';
import * as gulp from 'gulp';
import * as sourcemaps from 'gulp-sourcemaps';
import * as ts from 'gulp-typescript';

function goodReporter(): ts.reporter.Reporter {
  return {
    error: (error, typescript) => {
      if (error.tsFile) {
        console.log('[' + colors.gray('gulp-typescript') + '] ' + colors.red(error.fullFilename
          + '(' + (error.startPosition!.line + 1) + ',' + error.startPosition!.character + '): ')
          + 'error TS' + error.diagnostic.code + ': ' + typescript.flattenDiagnosticMessageText(error.diagnostic.messageText, '\n'));
      }
      else {
        console.log(error.message);
      }
    },
  };
}

const tsProject = ts.createProject('tsconfig.json');

export function compileTypeScript() {
  return tsProject.src()
    .pipe(sourcemaps.init())
    .pipe(tsProject(goodReporter()))
    .pipe(sourcemaps.write('.', {
      includeContent: false,
      sourceRoot: '.',
    }))
    .pipe(gulp.dest('out'));
}

export function watchTypeScript() {
  gulp.watch('src/**/*.ts', compileTypeScript);
}

/** Copy CSS files for the results view into the output directory. */
export function copyViewCss() {
  return gulp.src('src/view/*.css')
    .pipe(gulp.dest('out'));
}
