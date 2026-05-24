import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  root: '.',
  plugins: [react()],
  build: {
    outDir: 'dist',
    sourcemap: true,
    minify: 'esbuild',
    target: 'esnext'
  },
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:9306',
        changeOrigin: true
      },
      '/ws': {
        target: 'ws://localhost:9306',
        ws: true
      }
    }
  },
  resolve: {
    alias: {
      '@': '/src'
    }
  }
});

