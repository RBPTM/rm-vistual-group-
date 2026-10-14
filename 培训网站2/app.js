(() => {
  'use strict';

  const chapters = window.COURSE;
  const lessons = chapters.flatMap((chapter, chapterIndex) =>
    chapter.lessons.map((lesson, chapterLessonIndex) => ({
      ...lesson,
      chapterIndex,
      chapterLessonIndex,
      chapterTitle: chapter.title,
    })),
  );
  const byId = new Map(lessons.map((lesson) => [lesson.id, lesson]));
  const completedKey = 'rm-vision-course-completed-v1';
  const readingKey = 'rm-vision-course-reading-size-v1';
  const dom = (id) => document.getElementById(id);
  let activeId = '';
  let toastTimer;
  let completed = loadCompleted();

  function loadCompleted() {
    try {
      const saved = JSON.parse(localStorage.getItem(completedKey) || '[]');
      return new Set(Array.isArray(saved) ? saved.filter((id) => byId.has(id)) : []);
    } catch {
      return new Set();
    }
  }

  function saveCompleted() {
    try {
      localStorage.setItem(completedKey, JSON.stringify([...completed]));
    } catch {
      showToast('浏览器未能保存学习进度。');
    }
  }

  function safeText(value) {
    return String(value).replace(/[&<>"']/g, (character) => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
    })[character]);
  }

  function renderSidebar() {
    dom('course-nav').innerHTML = chapters.map((chapter, chapterIndex) => `
      <details class="chapter" data-chapter="${chapterIndex}" ${chapterIndex === 0 || chapter.lessons.some((item) => item.id === activeId) ? 'open' : ''}>
        <summary>${safeText(chapter.title)}</summary>
        ${chapter.lessons.map((lesson) => `
          <a class="lesson-link${lesson.id === activeId ? ' active' : ''}" data-lesson-id="${safeText(lesson.id)}" href="#${safeText(lesson.id)}"${lesson.id === activeId ? ' aria-current="page"' : ''}>
            <span class="lesson-name">${safeText(lesson.title)}</span>
            ${lesson.optional ? '<span class="optional-dot">选学</span>' : ''}
            ${completed.has(lesson.id) ? '<span class="done-dot" aria-label="已完成">✓</span>' : ''}
          </a>`).join('')}
      </details>`).join('');

    dom('course-nav').querySelectorAll('[data-lesson-id]').forEach((link) => {
      link.addEventListener('click', () => closeMobileMenu());
    });
    updateProgress();
  }

  function updateProgress() {
    const progress = dom('progress');
    progress.max = lessons.length;
    progress.value = completed.size;
    dom('progress-label').textContent = `${completed.size} / ${lessons.length}`;
  }

  function renderCodeBlocks() {
    dom('article').querySelectorAll('pre').forEach((pre) => {
      if (pre.parentElement.classList.contains('code-block')) return;
      const code = pre.querySelector('code');
      if (!code) return;
      const codeLanguage = code.className.match(/(?:^|\s)language-([^\s]+)/)?.[1];
      const language = safeText(pre.dataset.lang || codeLanguage || '代码');
      const wrapper = document.createElement('div');
      wrapper.className = 'code-block';
      const header = document.createElement('div');
      header.className = 'code-header';
      const label = document.createElement('span');
      label.textContent = language;
      const copy = document.createElement('button');
      copy.className = 'copy-button';
      copy.type = 'button';
      copy.textContent = '复制代码';
      copy.setAttribute('aria-label', `复制${language}代码`);
      copy.addEventListener('click', () => copyCode(code.textContent, copy));
      header.append(label, copy);
      pre.before(wrapper);
      wrapper.append(header, pre);
    });
  }

  async function copyCode(text, button) {
    try {
      if (navigator.clipboard && window.isSecureContext) {
        await navigator.clipboard.writeText(text);
      } else {
        const field = document.createElement('textarea');
        field.value = text;
        field.style.position = 'fixed';
        field.style.opacity = '0';
        document.body.append(field);
        field.select();
        const copied = document.execCommand('copy');
        field.remove();
        if (!copied) throw new Error('copy failed');
      }
      button.textContent = '已复制';
      window.setTimeout(() => { button.textContent = '复制代码'; }, 1600);
    } catch {
      showToast('复制失败，可以选中代码后手动复制。');
    }
  }

  function makePageOutline() {
    const headingLinks = [...dom('article').querySelectorAll('.lesson-body h2, .lesson-body h3, .lesson-body h4')];
    dom('page-outline').innerHTML = headingLinks.map((heading, index) => {
      const id = `${activeId}-section-${index + 1}`;
      heading.id = id;
      return `<a href="#${id}">${safeText(heading.textContent)}</a>`;
    }).join('');

    const links = [...dom('page-outline').querySelectorAll('a')];
    if (!('IntersectionObserver' in window)) return;
    const observer = new IntersectionObserver((entries) => {
      for (const entry of entries) {
        if (!entry.isIntersecting) continue;
        links.forEach((link) => link.classList.toggle('active', link.hash === `#${entry.target.id}`));
      }
    }, { rootMargin: '-15% 0px -72% 0px' });
    headingLinks.forEach((heading) => observer.observe(heading));
  }

  function renderLesson(id) {
    const lesson = byId.get(id) || lessons[0];
    activeId = lesson.id;
    const index = lessons.indexOf(lesson);
    dom('article').innerHTML = `
      <div class="lesson-eyebrow">${safeText(lesson.chapterTitle)}</div>
      <h1>${safeText(lesson.title)}</h1>
      <div class="lesson-body">${lesson.body}</div>`;

    const chapterDetails = [...dom('course-nav').querySelectorAll('.chapter')];
    chapterDetails.forEach((detail, detailIndex) => {
      detail.open = detailIndex === lesson.chapterIndex || detail.open;
    });
    dom('breadcrumb').textContent = `${lesson.chapterTitle} / ${lesson.title}`;
    dom('lesson-position').textContent = `第 ${index + 1} 节，共 ${lessons.length} 节`;
    const completedButton = dom('complete-button');
    const isComplete = completed.has(lesson.id);
    completedButton.setAttribute('aria-pressed', String(isComplete));
    dom('complete-label').textContent = isComplete ? '本节已完成' : '标记本节已完成';

    setPageLink(dom('previous-link'), lessons[index - 1], '← 上一节');
    setPageLink(dom('next-link'), lessons[index + 1], '下一节 →');
    renderSidebar();
    renderCodeBlocks();
    makePageOutline();
    dom('article').focus({ preventScroll: true });
    window.scrollTo({ top: 0, behavior: 'instant' });
  }

  function setPageLink(link, lesson, direction) {
    if (!lesson) {
      link.removeAttribute('href');
      link.hidden = true;
      return;
    }
    link.hidden = false;
    link.href = `#${lesson.id}`;
    link.querySelector('small').textContent = direction;
    link.querySelector('strong').textContent = lesson.title;
  }

  function showToast(message) {
    const toast = dom('toast');
    toast.textContent = message;
    toast.hidden = false;
    window.clearTimeout(toastTimer);
    toastTimer = window.setTimeout(() => { toast.hidden = true; }, 2500);
  }

  function closeMobileMenu() {
    if (!document.body.classList.contains('sidebar-open')) return;
    document.body.classList.remove('sidebar-open');
    document.body.classList.remove('sidebar-collapsed');
    dom('menu-button').setAttribute('aria-expanded', 'false');
    dom('mobile-shade').hidden = true;
  }

  function openSearchResults(query) {
    const container = dom('course-nav');
    const status = dom('search-status');
    const term = query.trim().toLocaleLowerCase();
    if (!term) {
      status.hidden = true;
      renderSidebar();
      return;
    }

    const matches = lessons.filter((lesson) => {
      const searchable = `${lesson.title} ${lesson.body.replace(/<[^>]*>/g, ' ')}`;
      return searchable.toLocaleLowerCase().includes(term);
    });
    status.hidden = false;
    status.textContent = `找到 ${matches.length} 节相关内容`;
    if (!matches.length) {
      container.innerHTML = '<p class="empty-search">没有找到匹配的章节。<br>试试“图片”“C++”或“视频”。</p>';
      return;
    }

    container.innerHTML = matches.map((lesson) => {
      const text = lesson.body.replace(/<[^>]*>/g, ' ').replace(/\s+/g, ' ');
      const position = text.toLocaleLowerCase().indexOf(term);
      const start = Math.max(0, position - 45);
      const excerpt = `${start ? '…' : ''}${text.slice(start, position + term.length + 65)}${position + term.length + 65 < text.length ? '…' : ''}`;
      const escaped = safeText(excerpt);
      const highlighted = escaped.replace(new RegExp(escapeRegExp(safeText(query.trim())), 'giu'), '<mark>$&</mark>');
      return `<div class="search-hit"><a class="lesson-link" href="#${safeText(lesson.id)}"><span class="lesson-name">${safeText(lesson.title)}</span></a><p>${highlighted}</p></div>`;
    }).join('');

    container.querySelectorAll('a[href]').forEach((link) => link.addEventListener('click', closeMobileMenu));
  }

  function escapeRegExp(value) {
    return value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  }

  function toggleSidebar() {
    if (window.matchMedia('(max-width: 760px)').matches) {
      const isOpen = document.body.classList.toggle('sidebar-open');
      dom('menu-button').setAttribute('aria-expanded', String(isOpen));
      dom('mobile-shade').hidden = !isOpen;
      return;
    }
    const collapsed = document.body.classList.toggle('sidebar-collapsed');
    dom('menu-button').setAttribute('aria-expanded', String(!collapsed));
  }

  function init() {
    try {
      const savedSize = Number(localStorage.getItem(readingKey));
      if (savedSize >= 14 && savedSize <= 20) document.documentElement.style.setProperty('--reading-size', `${savedSize}px`);
    } catch { /* Keep the default type size when storage is unavailable. */ }

    renderSidebar();
    const initialHash = decodeURIComponent(window.location.hash.slice(1));
    const initialLesson = byId.has(initialHash)
      ? initialHash
      : lessons.find((lesson) => initialHash.startsWith(`${lesson.id}-section-`))?.id || 'intro';
    renderLesson(initialLesson);
    if (initialHash && initialHash !== initialLesson && document.getElementById(initialHash)) {
      window.requestAnimationFrame(() => document.getElementById(initialHash)?.scrollIntoView());
    }
    window.addEventListener('hashchange', () => {
      const hash = decodeURIComponent(window.location.hash.slice(1));
      if (!hash) {
        renderLesson('intro');
      } else if (byId.has(hash)) {
        renderLesson(hash);
      }
    });
    dom('search').addEventListener('input', (event) => openSearchResults(event.target.value));
    dom('menu-button').addEventListener('click', toggleSidebar);
    dom('mobile-shade').addEventListener('click', closeMobileMenu);
    dom('print-button').addEventListener('click', () => window.print());
    dom('complete-button').addEventListener('click', () => {
      if (completed.has(activeId)) completed.delete(activeId);
      else completed.add(activeId);
      saveCompleted();
      renderLesson(activeId);
    });

    for (const [buttonId, change] of [['font-down', -1], ['font-up', 1]]) {
      dom(buttonId).addEventListener('click', () => {
        const current = parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--reading-size')) || 16;
        const next = Math.max(14, Math.min(20, current + change));
        document.documentElement.style.setProperty('--reading-size', `${next}px`);
        try { localStorage.setItem(readingKey, String(next)); } catch { /* Type size is still changed for this visit. */ }
      });
    }

    document.addEventListener('keydown', (event) => {
      if (event.key === '/' && !['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName)) {
        event.preventDefault();
        dom('search').focus();
      }
      if (event.key === 'Escape') closeMobileMenu();
    });
  }

  init();
})();
