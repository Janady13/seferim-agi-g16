#!/usr/bin/env node
// Multi-API Knowledge Feeder for SEFERIM AGI
// Fetches from multiple sources: Wikipedia, News, Quotes, Facts, etc.

const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const OUTPUT_FILE = process.env.AGI_STREAM_INDEX ||
  path.join(__dirname, '..', 'data', 'streaming_index.jsonl');

const FAMILY_NAMES = ['Athena', 'Iris', 'Hermes', 'Prometheus', 'Apollo', 'Hephaestus',
  'Demeter', 'Poseidon', 'Hera', 'Ares', 'Yad', 'Lashon', 'Chronos', 'Mnemosyne',
  'Hypnos', 'Thanatos'];

// Family mapping based on content/source
const SOURCE_FAMILIES = {
  wikipedia: 11,      // Lashon - Language/Knowledge
  news: 2,            // Hermes - Communication
  quotes: 4,          // Apollo - Harmony/Wisdom
  facts: 13,          // Mnemosyne - Memory
  programming: 10,    // Yad - Programming
  science: 3,         // Prometheus - Creation/Innovation
  history: 12,        // Chronos - Time
  nature: 6,          // Demeter - Growth/Nature
  sports: 9,          // Ares - Competition
  art: 1,             // Iris - Vision/Art
  business: 0,        // Athena - Strategy
  technology: 5,      // Hephaestus - Craft
};

// Keyword-based family detection
function detectFamily(text, defaultFamily = 11) {
  const lower = text.toLowerCase();

  const patterns = [
    [/\b(code|program|software|algorithm|function|api|javascript|python|rust)\b/, 10],
    [/\b(war|battle|military|army|fight|combat|sport|game|competition)\b/, 9],
    [/\b(art|painting|sculpture|museum|gallery|visual|design)\b/, 1],
    [/\b(music|song|album|band|composer|symphony|melody)\b/, 4],
    [/\b(plant|animal|nature|biology|ecology|species|forest)\b/, 6],
    [/\b(ocean|sea|water|river|marine|ship|sailing)\b/, 7],
    [/\b(government|law|politics|election|parliament|senate)\b/, 8],
    [/\b(history|century|ancient|medieval|historical|era)\b/, 12],
    [/\b(memory|remember|archive|record|database)\b/, 13],
    [/\b(science|research|experiment|discovery|physics|chemistry)\b/, 3],
    [/\b(business|company|market|economy|finance|trade)\b/, 0],
    [/\b(technology|engineering|machine|device|innovation)\b/, 5],
    [/\b(news|report|journalist|media|broadcast)\b/, 2],
  ];

  for (const [pattern, family] of patterns) {
    if (pattern.test(lower)) return family;
  }
  return defaultFamily;
}

// HTTP/HTTPS request helper
function fetch(url, options = {}) {
  return new Promise((resolve, reject) => {
    const client = url.startsWith('https') ? https : http;
    const req = client.get(url, {
      headers: { 'User-Agent': 'SEFERIM-Feeder/2.0', ...options.headers },
      timeout: 10000
    }, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        return fetch(res.headers.location, options).then(resolve).catch(reject);
      }
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => resolve({ status: res.statusCode, data }));
    });
    req.on('error', reject);
    req.on('timeout', () => { req.destroy(); reject(new Error('timeout')); });
  });
}

// === API SOURCES ===

// 1. Wikipedia Random Article
async function fetchWikipedia() {
  const url = 'https://en.wikipedia.org/w/api.php?action=query&generator=random&grnnamespace=0&grnlimit=1&prop=extracts&explaintext=1&exlimit=1&format=json&formatversion=2';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  const page = json.query?.pages?.[0];
  if (!page || !page.extract) return null;
  return {
    source: 'wikipedia',
    title: page.title,
    content: page.extract,
    family: detectFamily(page.extract, SOURCE_FAMILIES.wikipedia)
  };
}

// 2. Useless Facts API
async function fetchUselessFact() {
  const url = 'https://uselessfacts.jsph.pl/api/v2/facts/random?language=en';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.text) return null;
  return {
    source: 'facts',
    title: 'Random Fact',
    content: json.text,
    family: detectFamily(json.text, SOURCE_FAMILIES.facts)
  };
}

// 3. Quotes API (quotable.io)
async function fetchQuote() {
  const url = 'https://api.quotable.io/random';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.content) return null;
  return {
    source: 'quotes',
    title: `Quote by ${json.author}`,
    content: `"${json.content}" - ${json.author}`,
    family: SOURCE_FAMILIES.quotes
  };
}

// 4. Numbers API (trivia about numbers)
async function fetchNumberTrivia() {
  const num = Math.floor(Math.random() * 1000);
  const types = ['trivia', 'math', 'date', 'year'];
  const type = types[Math.floor(Math.random() * types.length)];
  const url = `http://numbersapi.com/${num}/${type}`;
  const res = await fetch(url);
  if (!res.data) return null;
  return {
    source: 'numbers',
    title: `Number ${num} (${type})`,
    content: res.data,
    family: type === 'math' ? 10 : type === 'date' || type === 'year' ? 12 : 13
  };
}

// 5. Cat Facts (fun facts)
async function fetchCatFact() {
  const url = 'https://catfact.ninja/fact';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.fact) return null;
  return {
    source: 'catfacts',
    title: 'Cat Fact',
    content: json.fact,
    family: SOURCE_FAMILIES.nature
  };
}

// 6. Dog Facts
async function fetchDogFact() {
  const url = 'https://dogapi.dog/api/v2/facts?limit=1';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  const fact = json.data?.[0]?.attributes?.body;
  if (!fact) return null;
  return {
    source: 'dogfacts',
    title: 'Dog Fact',
    content: fact,
    family: SOURCE_FAMILIES.nature
  };
}

// 7. Bored API (activity suggestions)
async function fetchActivity() {
  const url = 'https://www.boredapi.com/api/activity';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.activity) return null;
  return {
    source: 'activities',
    title: `Activity: ${json.type}`,
    content: `${json.activity}. Type: ${json.type}, Participants: ${json.participants}, Price: ${json.price}`,
    family: detectFamily(json.activity + ' ' + json.type, 14)
  };
}

// 8. Advice Slip
async function fetchAdvice() {
  const url = 'https://api.adviceslip.com/advice';
  const res = await fetch(url);
  // Remove JSONP wrapper if present
  const clean = res.data.replace(/^[^{]*/, '').replace(/[^}]*$/, '');
  const json = JSON.parse(clean);
  if (!json.slip?.advice) return null;
  return {
    source: 'advice',
    title: 'Life Advice',
    content: json.slip.advice,
    family: SOURCE_FAMILIES.quotes
  };
}

// 9. Chuck Norris Jokes (humor/entertainment)
async function fetchChuckNorris() {
  const url = 'https://api.chucknorris.io/jokes/random';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.value) return null;
  return {
    source: 'chucknorris',
    title: 'Chuck Norris Fact',
    content: json.value,
    family: 9 // Ares - competition/strength
  };
}

// 10. Official Joke API
async function fetchJoke() {
  const url = 'https://official-joke-api.appspot.com/random_joke';
  const res = await fetch(url);
  const json = JSON.parse(res.data);
  if (!json.setup) return null;
  return {
    source: 'jokes',
    title: `Joke (${json.type})`,
    content: `${json.setup} ${json.punchline}`,
    family: 4 // Apollo - harmony/entertainment
  };
}

// All fetchers
const FETCHERS = [
  { name: 'Wikipedia', fn: fetchWikipedia, weight: 5 },
  { name: 'Facts', fn: fetchUselessFact, weight: 2 },
  { name: 'Quotes', fn: fetchQuote, weight: 2 },
  { name: 'Numbers', fn: fetchNumberTrivia, weight: 2 },
  { name: 'CatFacts', fn: fetchCatFact, weight: 1 },
  { name: 'DogFacts', fn: fetchDogFact, weight: 1 },
  { name: 'Activities', fn: fetchActivity, weight: 1 },
  { name: 'Advice', fn: fetchAdvice, weight: 1 },
  { name: 'ChuckNorris', fn: fetchChuckNorris, weight: 1 },
  { name: 'Jokes', fn: fetchJoke, weight: 1 },
];

// Weighted random selection
function selectFetcher() {
  const totalWeight = FETCHERS.reduce((sum, f) => sum + f.weight, 0);
  let rand = Math.random() * totalWeight;
  for (const fetcher of FETCHERS) {
    rand -= fetcher.weight;
    if (rand <= 0) return fetcher;
  }
  return FETCHERS[0];
}

// Write to index file
function writeItem(item) {
  const line = JSON.stringify({
    path: `${item.source}/${item.title.replace(/[^a-zA-Z0-9]/g, '_').slice(0, 50)}.txt`,
    type: 'txt',
    content: item.content.slice(0, 4000), // Limit content size
    family: item.family
  });
  fs.appendFileSync(OUTPUT_FILE, line + '\n');
}

// Stats tracking
const stats = {
  total: 0,
  bySource: {},
  byFamily: {},
  errors: 0,
  startTime: Date.now()
};

function updateStats(item) {
  stats.total++;
  stats.bySource[item.source] = (stats.bySource[item.source] || 0) + 1;
  stats.byFamily[item.family] = (stats.byFamily[item.family] || 0) + 1;
}

function printStats() {
  const elapsed = Math.round((Date.now() - stats.startTime) / 1000);
  console.log('\n--- Stats ---');
  console.log(`Total: ${stats.total} items in ${elapsed}s (${(stats.total / elapsed * 60).toFixed(1)}/min)`);
  console.log(`Errors: ${stats.errors}`);
  console.log('By Source:', stats.bySource);
  console.log('By Family:', Object.entries(stats.byFamily).map(([k, v]) => `${FAMILY_NAMES[k]}:${v}`).join(', '));
  console.log('-------------\n');
}

async function main() {
  console.log('SEFERIM Multi-API Knowledge Feeder v2.0');
  console.log('=======================================');
  console.log(`Output: ${OUTPUT_FILE}`);
  console.log(`Sources: ${FETCHERS.map(f => f.name).join(', ')}`);
  console.log('Fetching knowledge continuously...\n');

  // Don't clear file - append to preserve history
  if (!fs.existsSync(OUTPUT_FILE)) {
    fs.writeFileSync(OUTPUT_FILE, '');
  }

  // Print stats every 30 items
  setInterval(() => {
    if (stats.total > 0 && stats.total % 30 === 0) printStats();
  }, 1000);

  while (true) {
    const fetcher = selectFetcher();
    try {
      const item = await fetcher.fn();
      if (item && item.content && item.content.length > 20) {
        writeItem(item);
        updateStats(item);
        const timestamp = new Date().toLocaleTimeString();
        const familyName = FAMILY_NAMES[item.family] || `F${item.family}`;
        console.log(`[${timestamp}] #${stats.total} [${fetcher.name}] "${item.title.slice(0, 35)}" -> ${familyName} (${item.content.length} chars)`);
      }
    } catch (e) {
      stats.errors++;
      // Silent fail - just try another source
    }

    // Random delay 1-3 seconds
    await new Promise(r => setTimeout(r, 1000 + Math.random() * 2000));
  }
}

// Handle shutdown gracefully
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  printStats();
  process.exit(0);
});

process.on('SIGTERM', () => {
  printStats();
  process.exit(0);
});

main().catch(console.error);
