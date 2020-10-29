import * as path from 'path';
import * as webpack from 'webpack';

export const config: webpack.Configuration = {
  mode: 'development',
  entry: {
    resultsView: './src/view/results.tsx',
    compareView: './src/compare/view/Compare.tsx',
  },
  output: {
    path: path.resolve(__dirname, '..', 'out'),
    filename: '[name].js'
  },
  devtool: 'inline-source-map',
  resolve: {
    extensions: ['.js', '.ts', '.tsx', '.json']
  },
  module: {
    rules: [
      {
        test: /\.(ts|tsx)$/,
        loader: 'ts-loader',
        options: {
          configFile: 'src/view/tsconfig.json',
        }
      },
      {
        test: /\.less$/,
        use: [
          {
            loader: 'style-loader'
          },
          {
            loader: 'css-loader',
            options: {
              importLoaders: 1,
              sourceMap: true
            }
          },
          {
            loader: 'less-loader',
            options: {
              javascriptEnabled: true,
              sourceMap: true
            }
          }
        ]
      },
      {
        test: /\.css$/,
        use: [
          {
            loader: 'style-loader'
          },
          {
            loader: 'css-loader'
          }
        ]
      }
    ]
  },
  performance: {
    hints: false
  }
};
