/**
 * Airgap JSON Formatter - Service Worker
 * Enables offline functionality by caching all application assets
 */

const CACHE_NAME = 'airgap-json-formatter-v2';

// Assets to precache for offline use (relative paths for GitHub Pages compatibility)
const PRECACHE_ASSETS = [
    './',
    './index.html',
    './bridge.js',
    './history-storage.js',
    './pkg/airgap_json_formatter.js',
    './pkg/airgap_json_formatter_bg.wasm',
    './manifest.json',
    './airgap_formatter.js',
    './airgap_formatter.wasm'
];

// Install event - precache all assets
self.addEventListener('install', (event) => {
    console.log('[SW] Installing Service Worker...');
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then((cache) => {
                console.log('[SW] Precaching assets');
                // Cache assets that exist, skip missing ones
                return Promise.allSettled(
                    PRECACHE_ASSETS.map(url =>
                        cache.add(url).catch(err => {
                            console.warn(`[SW] Failed to cache ${url}:`, err.message);
                        })
                    )
                );
            })
            .then(() => {
                console.log('[SW] Installation complete');
                return self.skipWaiting();
            })
    );
});

// Activate event - clean up old caches
self.addEventListener('activate', (event) => {
    console.log('[SW] Activating Service Worker...');
    event.waitUntil(
        caches.keys()
            .then((cacheNames) => {
                return Promise.all(
                    cacheNames
                        .filter((name) => name.startsWith('airgap-json-formatter-') && name !== CACHE_NAME)
                        .map((name) => {
                            console.log('[SW] Deleting old cache:', name);
                            return caches.delete(name);
                        })
                );
            })
            .then(() => {
                console.log('[SW] Activation complete');
                return self.clients.claim();
            })
    );
});

// Fetch event - serve from cache, fallback to network
self.addEventListener('fetch', (event) => {
    // Only handle GET requests
    if (event.request.method !== 'GET') return;

    // Skip cross-origin requests
    if (!event.request.url.startsWith(self.location.origin)) return;

    event.respondWith(
        caches.match(event.request)
            .then((cachedResponse) => {
                if (cachedResponse) {
                    return cachedResponse;
                }

                // Not in cache - fetch from network
                return fetch(event.request)
                    .then((response) => {
                        // Don't cache non-successful responses
                        if (!response || response.status !== 200) {
                            return response;
                        }

                        // Clone and cache the response
                        const responseToCache = response.clone();
                        caches.open(CACHE_NAME)
                            .then((cache) => {
                                cache.put(event.request, responseToCache);
                            });

                        return response;
                    })
                    .catch(() => {
                        // Network failed and not in cache
                        // Return a basic offline response for HTML requests
                        if (event.request.headers.get('accept').includes('text/html')) {
                            return new Response(
                                '<html><body><h1>Offline</h1><p>Please connect to the internet and reload.</p></body></html>',
                                { headers: { 'Content-Type': 'text/html' } }
                            );
                        }
                    });
            })
    );
});

// Listen for update messages from main thread
self.addEventListener('message', (event) => {
    if (event.data && event.data.type === 'SKIP_WAITING') {
        self.skipWaiting();
    }
});
