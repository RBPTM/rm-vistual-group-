'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const context = { window: {} };
vm.runInNewContext(fs.readFileSync(path.join(__dirname, 'content.js'), 'utf8'), context);
const chapters = context.window.COURSE;
const lessons = chapters.flatMap((chapter) => chapter.lessons);
const titles = Array.from(lessons, (lesson) => lesson.title);
const colorIndex = lessons.findIndex((lesson) => lesson.id === 'color-spaces');

assert.equal(chapters.length, 1);
assert.equal(lessons.length, 12);
assert.deepEqual(titles.slice(colorIndex - 1, colorIndex + 2), [
  '5. CMake 工程与构建',
  '6. 图像的基础知识：BGR、HSV、LAB、灰度图',
  '7. OpenCV 图像基础：cv::Mat、输入输出与像素操作',
]);
assert.ok(titles.some((title) => title.startsWith('8. OpenCV 视频基础')));
assert.ok(titles.some((title) => title.startsWith('9. 第一周综合练习')));
assert.equal(new Set(lessons.map((lesson) => lesson.id)).size, lessons.length);

const opencvLesson = lessons.find((lesson) => lesson.title.startsWith('7. OpenCV 图像基础'));
assert.ok(opencvLesson.body.includes('OpenCV</strong> 是一个开源的计算机视觉与图像处理工具库'));
assert.ok(opencvLesson.body.includes('不是一种编程语言'));
assert.ok(opencvLesson.body.includes('<code>cv::Mat</code>'));

const colorLesson = lessons[colorIndex];
const imagePath = path.join(__dirname, 'assets', 'color-spaces.png');
assert.ok(fs.statSync(imagePath).size > 1000);
assert.equal((colorLesson.body.match(/src="assets\/color-spaces.png"/g) || []).length, 2);
assert.ok(colorLesson.body.includes('grayscale-example'));
for (const concept of ['BGR', 'HSV', 'Lab', '灰度图', '红叶', '石路']) {
  assert.ok(colorLesson.body.includes(concept), `Missing ${concept}`);
}
assert.ok((colorLesson.body.match(/<h2>/g) || []).length >= 6);
assert.ok(!/<pre\b|<code\b|COLOR_BGR2|单通道视图/.test(colorLesson.body));

const overview = lessons.find((lesson) => lesson.title.startsWith('1. 总体路线'));
assert.ok(overview.body.includes('第 7 章'));
assert.ok(overview.body.includes('第 8 章'));

const intro = lessons.find((lesson) => lesson.id === 'intro');
for (const image of ['armor-pairing.png']) {
  assert.ok(fs.statSync(path.join(__dirname, 'assets', image)).size > 1000);
  assert.ok(overview.body.includes(`src="assets/${image}"`));
}
assert.ok(overview.body.includes('算法组在做什么'));
assert.ok(overview.body.includes('不是本周培训承诺完成的内容'));
assert.ok(!overview.body.includes('showcase-source'));
assert.ok(!overview.body.includes('github.com/'));
assert.ok(!intro.body.includes('armor-pairing.png'));
console.log('Course order, section content, overview showcase, and offline images verified.');
