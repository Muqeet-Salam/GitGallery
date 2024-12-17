const form = document.getElementById('repo-form');
const input = document.getElementById('repo-input');
const gallery = document.getElementById('gallery');
const status = document.getElementById('status');
const breadcrumbs = document.getElementById('breadcrumbs');
const folderList = document.getElementById('folder-list');
const recursiveToggle = document.getElementById('recursive-toggle');
const prevPageBtn = document.getElementById('prev-page');
const nextPageBtn = document.getElementById('next-page');
const pageInfo = document.getElementById('page-info');

let activeRepo = '';
let activePath = '';
let activePage = 1;
let activeTotalPages = 0;
const PAGE_SIZE = 50;

function createPathPart(path, index) {
  const parts = path.split('/').filter(Boolean);
  return parts.slice(0, index + 1).join('/');
}

function renderBreadcrumbs(path) {
  breadcrumbs.innerHTML = '';

  const rootButton = document.createElement('button');
  rootButton.className = 'crumb';
  rootButton.textContent = 'root';
  rootButton.addEventListener('click', () => loadRepo(activeRepo, ''));
  breadcrumbs.appendChild(rootButton);

  const parts = path.split('/').filter(Boolean);
  parts.forEach((part, idx) => {
    const divider = document.createElement('span');
    divider.className = 'crumb-divider';
    divider.textContent = '/';
    breadcrumbs.appendChild(divider);

    const partButton = document.createElement('button');
    partButton.className = 'crumb';
    partButton.textContent = part;
    partButton.addEventListener('click', () => loadRepo(activeRepo, createPathPart(path, idx)));
    breadcrumbs.appendChild(partButton);
  });
}

function renderFolders(folders) {
  folderList.innerHTML = '';
  if (!Array.isArray(folders) || folders.length === 0) {
    folderList.textContent = 'No subfolders in this path.';
    return;
  }

  folders.forEach((folder) => {
    const btn = document.createElement('button');
    btn.className = 'folder-item';
    btn.textContent = `[DIR] ${folder.name}`;
    btn.addEventListener('click', () => loadRepo(activeRepo, folder.path));
    folderList.appendChild(btn);
  });
}

function renderImages(images) {
  gallery.innerHTML = '';

  if (!Array.isArray(images) || images.length === 0) {
    return;
  }

  images.forEach((img) => {
    const container = document.createElement('div');
    container.className = 'photo';

    const image = document.createElement('img');
    image.src = img.download_url;
    image.alt = img.name || 'image';
    image.loading = 'lazy';

    const label = document.createElement('div');
    label.className = 'photo-label';
    label.textContent = img.path || img.name;

    container.appendChild(image);
    container.appendChild(label);
    gallery.appendChild(container);
  });
}

function renderPagination(pagination) {
  const page = Number(pagination?.page || 1);
  const totalPages = Number(pagination?.total_pages || 0);
  const totalImages = Number(pagination?.total_images || 0);

  activePage = page;
  activeTotalPages = totalPages;

  if (totalImages === 0) {
    pageInfo.textContent = 'No pages';
    prevPageBtn.disabled = true;
    nextPageBtn.disabled = true;
    status.textContent = 'No images found in the selected path.';
    return;
  }

  pageInfo.textContent = `Page ${page} of ${totalPages || 1}`;
  prevPageBtn.disabled = page <= 1;
  nextPageBtn.disabled = totalPages === 0 || page >= totalPages;
  status.textContent = `${totalImages} total image(s). Showing ${PAGE_SIZE} per page.`;
}

async function loadRepo(repo, path = '', page = 1) {
  if (!repo) return;
  activeRepo = repo;
  activePath = path;
  activePage = page;
  status.textContent = 'Loading...';

  try {
    const params = new URLSearchParams();
    params.set('repo', repo);
    if (path) params.set('path', path);
    params.set('recursive', recursiveToggle.checked ? '1' : '0');
    params.set('page', String(page));
    params.set('per_page', String(PAGE_SIZE));

    const r = await fetch(`/api/images?${params.toString()}`);
    if (!r.ok) throw new Error(`Request failed ${r.status}`);
    const payload = await r.json();

    const effectivePath = payload.path || path || '';
    renderBreadcrumbs(effectivePath);
    renderFolders(payload.folders || []);
    renderImages(payload.images || []);
    renderPagination(payload.pagination || {});
  } catch (err) {
    console.error(err);
    status.textContent = 'Could not fetch repository data. Check console.';
  }
}

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  const repo = input.value.trim();
  if (!repo) return;
  await loadRepo(repo, '', 1);
});

recursiveToggle.addEventListener('change', () => {
  if (activeRepo)
    loadRepo(activeRepo, activePath, 1);
});

prevPageBtn.addEventListener('click', () => {
  if (!activeRepo || activePage <= 1) return;
  loadRepo(activeRepo, activePath, activePage - 1);
});

nextPageBtn.addEventListener('click', () => {
  if (!activeRepo || activeTotalPages === 0 || activePage >= activeTotalPages) return;
  loadRepo(activeRepo, activePath, activePage + 1);
});
