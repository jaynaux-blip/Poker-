import { defineConfig } from 'vitest/config';

export default defineConfig({
  // Relative asset paths so the build works from any folder or static host.
  base: './',
  build: {
    target: 'es2022',
    chunkSizeWarningLimit: 1500,
  },
  test: {
    include: ['tests/**/*.test.ts'],
  },
});
