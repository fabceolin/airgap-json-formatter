/**
 * IndexedDB storage layer for JSON history
 * Provides persistent storage for formatted JSON documents
 */

const DB_NAME = 'AirgapJsonFormatter';
const DB_VERSION = 1;
const STORE_NAME = 'history';
const MAX_ENTRIES = 100;

let db = null;

/**
 * Initialize the IndexedDB database
 * @returns {Promise<void>}
 */
async function initHistoryDB() {
    if (db) {
        return;
    }

    return new Promise((resolve, reject) => {
        const request = indexedDB.open(DB_NAME, DB_VERSION);

        request.onerror = () => {
            console.error('[History] Failed to open database:', request.error);
            reject(request.error);
        };

        request.onsuccess = () => {
            db = request.result;
            console.log('[History] Database initialized');
            resolve();
        };

        request.onupgradeneeded = (event) => {
            const database = event.target.result;

            if (!database.objectStoreNames.contains(STORE_NAME)) {
                const store = database.createObjectStore(STORE_NAME, { keyPath: 'id' });
                store.createIndex('timestamp', 'timestamp', { unique: false });
                store.createIndex('contentHash', 'contentHash', { unique: false });
                console.log('[History] Object store created');
            }
        };
    });
}

/**
 * Generate SHA-256 hash of content for deduplication
 * @param {string} content - Content to hash
 * @returns {Promise<string>} Hex hash string
 */
async function hashContent(content) {
    const encoder = new TextEncoder();
    const data = encoder.encode(content);
    const hashBuffer = await crypto.subtle.digest('SHA-256', data);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

/**
 * Find entry by content hash
 * @param {string} contentHash - Hash to search for
 * @returns {Promise<object|null>} Entry or null if not found
 */
async function findByHash(contentHash) {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readonly');
        const store = transaction.objectStore(STORE_NAME);
        const index = store.index('contentHash');
        const request = index.get(contentHash);

        request.onsuccess = () => resolve(request.result || null);
        request.onerror = () => reject(request.error);
    });
}

/**
 * Get count of history entries
 * @returns {Promise<number>}
 */
async function getCount() {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readonly');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.count();

        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
    });
}

/**
 * Get oldest entry by timestamp
 * @returns {Promise<object|null>}
 */
async function getOldestEntry() {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readonly');
        const store = transaction.objectStore(STORE_NAME);
        const index = store.index('timestamp');
        const request = index.openCursor();

        request.onsuccess = () => {
            const cursor = request.result;
            resolve(cursor ? cursor.value : null);
        };
        request.onerror = () => reject(request.error);
    });
}

/**
 * Enforce maximum entry limit by removing oldest entries
 * @returns {Promise<void>}
 */
async function enforceLimit() {
    const count = await getCount();

    if (count >= MAX_ENTRIES) {
        const oldest = await getOldestEntry();
        if (oldest) {
            await deleteEntry(oldest.id);
            // Recursively check if we need to delete more
            await enforceLimit();
        }
    }
}

/**
 * Add a new entry to history
 * @param {object} entry - Entry to add
 * @returns {Promise<void>}
 */
async function addEntry(entry) {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readwrite');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.add(entry);

        request.onsuccess = () => resolve();
        request.onerror = () => reject(request.error);
    });
}

/**
 * Update an existing entry
 * @param {object} entry - Entry to update
 * @returns {Promise<void>}
 */
async function updateEntry(entry) {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readwrite');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.put(entry);

        request.onsuccess = () => resolve();
        request.onerror = () => reject(request.error);
    });
}

/**
 * Delete an entry by ID
 * @param {string} id - Entry ID to delete
 * @returns {Promise<void>}
 */
async function deleteEntry(id) {
    if (!db) await initHistoryDB();

    return new Promise((resolve, reject) => {
        const transaction = db.transaction([STORE_NAME], 'readwrite');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.delete(id);

        request.onsuccess = () => resolve();
        request.onerror = () => reject(request.error);
    });
}

/**
 * History Storage API - exposed to window for Qt/WASM access
 */
const HistoryStorage = {
    /**
     * Initialize the storage (call on app startup)
     * @returns {Promise<boolean>}
     */
    async init() {
        try {
            await initHistoryDB();
            return true;
        } catch (e) {
            console.error('[History] Init failed:', e);
            return false;
        }
    },

    /**
     * Save JSON to history
     * @param {string} json - JSON content to save
     * @returns {Promise<{success: boolean, id?: string, error?: string}>}
     */
    async save(json) {
        try {
            if (!db) await initHistoryDB();

            const contentHash = await hashContent(json);
            const existing = await findByHash(contentHash);

            if (existing) {
                // Update timestamp only for duplicate content
                existing.timestamp = new Date().toISOString();
                await updateEntry(existing);
                return { success: true, id: existing.id, updated: true };
            }

            // Enforce limit before adding new entry
            await enforceLimit();

            const entry = {
                id: crypto.randomUUID(),
                content: json,
                contentHash: contentHash,
                timestamp: new Date().toISOString(),
                preview: json.substring(0, 100),
                size: new Blob([json]).size
            };

            await addEntry(entry);
            return { success: true, id: entry.id, updated: false };
        } catch (e) {
            console.error('[History] Save failed:', e);
            return { success: false, error: String(e) };
        }
    },

    /**
     * Load all history entries sorted by timestamp (newest first)
     * @returns {Promise<{success: boolean, entries?: Array, error?: string}>}
     */
    async loadAll() {
        try {
            if (!db) await initHistoryDB();

            return new Promise((resolve, reject) => {
                const transaction = db.transaction([STORE_NAME], 'readonly');
                const store = transaction.objectStore(STORE_NAME);
                const request = store.getAll();

                request.onsuccess = () => {
                    const entries = request.result || [];
                    // Sort by timestamp descending (newest first)
                    entries.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
                    resolve({ success: true, entries });
                };
                request.onerror = () => {
                    reject({ success: false, error: String(request.error) });
                };
            });
        } catch (e) {
            console.error('[History] LoadAll failed:', e);
            return { success: false, error: String(e) };
        }
    },

    /**
     * Get a single entry by ID
     * @param {string} id - Entry ID
     * @returns {Promise<{success: boolean, entry?: object, error?: string}>}
     */
    async get(id) {
        try {
            if (!db) await initHistoryDB();

            return new Promise((resolve, reject) => {
                const transaction = db.transaction([STORE_NAME], 'readonly');
                const store = transaction.objectStore(STORE_NAME);
                const request = store.get(id);

                request.onsuccess = () => {
                    if (request.result) {
                        resolve({ success: true, entry: request.result });
                    } else {
                        resolve({ success: false, error: 'Entry not found' });
                    }
                };
                request.onerror = () => {
                    reject({ success: false, error: String(request.error) });
                };
            });
        } catch (e) {
            console.error('[History] Get failed:', e);
            return { success: false, error: String(e) };
        }
    },

    /**
     * Delete a single entry by ID
     * @param {string} id - Entry ID to delete
     * @returns {Promise<{success: boolean, error?: string}>}
     */
    async delete(id) {
        try {
            await deleteEntry(id);
            return { success: true };
        } catch (e) {
            console.error('[History] Delete failed:', e);
            return { success: false, error: String(e) };
        }
    },

    /**
     * Clear all history entries
     * @returns {Promise<{success: boolean, error?: string}>}
     */
    async clear() {
        try {
            if (!db) await initHistoryDB();

            return new Promise((resolve, reject) => {
                const transaction = db.transaction([STORE_NAME], 'readwrite');
                const store = transaction.objectStore(STORE_NAME);
                const request = store.clear();

                request.onsuccess = () => resolve({ success: true });
                request.onerror = () => reject({ success: false, error: String(request.error) });
            });
        } catch (e) {
            console.error('[History] Clear failed:', e);
            return { success: false, error: String(e) };
        }
    },

    /**
     * Check if IndexedDB is available
     * @returns {boolean}
     */
    isAvailable() {
        return typeof indexedDB !== 'undefined';
    }
};

// Export to global scope for Qt access
window.HistoryStorage = HistoryStorage;

// ES module exports for modern usage
export { HistoryStorage, initHistoryDB };
