#!/usr/bin/env node
// Continuous Wikipedia article feeder for SEFERIM AGI
// Fetches random Wikipedia articles and appends to index file

const https = require('https');
const fs = require('fs');
const path = require('path');

const OUTPUT_FILE = process.env.AGI_STREAM_INDEX ||
  path.join(__dirname, '..', 'data', 'streaming_index.jsonl');

// Family mapping based on content type
const FAMILY_KEYWORDS = {
  0: ['strategy', 'planning', 'decision', 'policy', 'management', 'organization'],
  1: ['image', 'visual', 'art', 'design', 'photography', 'graphics'],
  2: ['communication', 'language', 'speech', 'media', 'journalism'],
  3: ['creation', 'invention', 'innovation', 'technology', 'engineering'],
  4: ['music', 'harmony', 'rhythm', 'sound', 'audio'],
  5: ['craft', 'manufacturing', 'production', 'building', 'construction'],
  6: ['growth', 'agriculture', 'biology', 'nature', 'ecology'],
  7: ['flow', 'water', 'ocean', 'fluid', 'dynamics'],
  8: ['governance', 'law', 'politics', 'government', 'regulation'],
  9: ['competition', 'war', 'conflict', 'sport', 'game'],
  10: ['programming', 'software', 'computer', 'algorithm', 'code'],
  11: ['language', 'linguistics', 'grammar', 'writing', 'literature'],
  12: ['time', 'history', 'chronology', 'calendar', 'schedule'],
  13: ['memory', 'archive', 'record', 'database', 'storage'],
  14: ['rest', 'sleep', 'relaxation', 'meditation', 'recovery'],
  15: ['death', 'end', 'termination', 'cleanup', 'removal']
};

function detectFamily(title, extract) {
  const text = (title + ' ' + extract).toLowerCase();
  let bestFamily = 11; // Default to Lashon (language)
  let bestScore = 0;

  for (const [fam, keywords] of Object.entries(FAMILY_KEYWORDS)) {
    let score = 0;
    for (const kw of keywords) {
      if (text.includes(kw)) score++;
    }
    if (score > bestScore) {
      bestScore = score;
      bestFamily = parseInt(fam);
    }
  }
  return bestFamily;
}

function fetchRandomArticle() {
  return new Promise((resolve, reject) => {
    const url = 'https://en.wikipedia.org/w/api.php?action=query&generator=random&grnnamespace=0&grnlimit=1&prop=extracts&explaintext=1&exlimit=1&format=json&formatversion=2';

    https.get(url, { headers: { 'User-Agent': 'SEFERIM-Feeder/1.0' } }, (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => {
        try {
          const json = JSON.parse(data);
          const pages = json.query?.pages;
          if (pages && pages.length > 0) {
            const page = pages[0];
            resolve({
              title: page.title,
              extract: page.extract || '',
              pageid: page.pageid
            });
          } else {
            reject(new Error('No pages in response'));
          }
        } catch (e) {
          reject(e);
        }
      });
    }).on('error', reject);
  });
}

async function writeArticle(article) {
  const family = detectFamily(article.title, article.extract);
  const line = JSON.stringify({
    path: `wiki/${article.title.replace(/ /g, '_')}.txt`,
    type: 'txt',
    content: article.extract.slice(0, 2000), // Limit content size
    family: family
  });

  fs.appendFileSync(OUTPUT_FILE, line + '\n');
  return { title: article.title, family, length: article.extract.length };
}

async function main() {
  console.log('SEFERIM Wikipedia Feeder');
  console.log('========================');
  console.log(`Output: ${OUTPUT_FILE}`);
  console.log('Fetching random articles continuously...\n');

  // Clear/create file
  fs.writeFileSync(OUTPUT_FILE, '');

  let count = 0;
  const familyNames = ['Athena', 'Iris', 'Hermes', 'Prometheus', 'Apollo', 'Hephaestus',
    'Demeter', 'Poseidon', 'Hera', 'Ares', 'Yad', 'Lashon', 'Chronos', 'Mnemosyne',
    'Hypnos', 'Thanatos'];

  while (true) {
    try {
      const article = await fetchRandomArticle();
      if (article.extract && article.extract.length > 100) {
        const result = await writeArticle(article);
        count++;
        const timestamp = new Date().toLocaleTimeString();
        console.log(`[${timestamp}] #${count} "${result.title.slice(0, 40)}" -> ${familyNames[result.family]} (${result.length} chars)`);
      }
    } catch (e) {
      console.error('Fetch error:', e.message);
    }

    // Wait 2 seconds between fetches to be nice to Wikipedia
    await new Promise(r => setTimeout(r, 2000));
  }
}

main().catch(console.error);
