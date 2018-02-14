const path = require('path');

module.exports = {
  entry: './src/Demo.bs.js',
  output: {
    path: path.resolve(__dirname, 'dist'),
    filename: 'index.js'
  }
};
