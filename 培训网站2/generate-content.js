'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const sourcePath = path.resolve(__dirname, '..', '专项培训_1.md');
const outputPath = path.join(__dirname, 'content.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const document = JSON.parse(execFileSync('pandoc', ['--from=markdown', '--to=json'], {
  input: source,
  encoding: 'utf8',
  maxBuffer: 8 * 1024 * 1024,
}));

function textOf(node) {
  if (node === null || node === undefined) return '';
  if (typeof node === 'string') return node;
  if (Array.isArray(node)) return node.map(textOf).join('');
  if (node.t === 'Space' || node.t === 'SoftBreak' || node.t === 'LineBreak') return ' ';
  if (node.t === 'Str' || node.t === 'Code') return typeof node.c === 'string' ? node.c : textOf(node.c);
  return textOf(node.c);
}

function renumberHeadings(blocks, replacements) {
  for (const block of blocks) {
    if (block.t !== 'Header') continue;
    const number = block.c[2].find((inline) => inline.t === 'Str' && replacements[inline.c]);
    if (number) number.c = replacements[number.c];
  }
}

const groups = [];
let current = [];
let title = '课程介绍';
for (const block of document.blocks) {
  if (block.t === 'Header' && block.c[0] === 2) {
    if (current.length) groups.push({ title, blocks: current });
    title = textOf(block.c[2]).trim();
    current = [];
  } else {
    current.push(block);
  }
}
if (current.length) groups.push({ title, blocks: current });

const ubuntuIndex = groups.findIndex((group) => group.title.startsWith('4. Ubuntu'));
if (ubuntuIndex >= 0) {
  const [environmentGroup] = groups.splice(ubuntuIndex, 1);
  const cmakeStart = environmentGroup.blocks.findIndex((block) =>
    block.t === 'Header' && block.c[0] === 3 && textOf(block.c[2]).trim().startsWith('4.2 建立工程'),
  );

  if (cmakeStart >= 0) {
    const ubuntuBlocks = environmentGroup.blocks.slice(0, cmakeStart);
    const cmakeBlocks = environmentGroup.blocks.slice(cmakeStart);
    renumberHeadings(ubuntuBlocks, { '4.1': '3.1', '4.1.1': '3.1.1' });
    renumberHeadings(cmakeBlocks, { '4.2': '5.1', '4.3': '5.2', '4.4': '5.3', '4.5': '5.4' });
    const ubuntuLesson = {
      title: '3. Ubuntu 环境与常用命令',
      blocks: ubuntuBlocks,
    };
    const cmakeLesson = {
      title: '5. CMake 工程与构建',
      blocks: cmakeBlocks,
    };
    const cppIndex = groups.findIndex((group) => group.title.startsWith('3. 从 C 到 C++'));
    const cppGroup = groups[cppIndex];
    if (cppGroup) {
      const cppNumbers = Object.fromEntries(
        ['3.1', '3.1.1', '3.1.2', '3.1.3', '3.2', '3.2.1', '3.3', '3.4', '3.5']
          .map((number) => [number, number.replace(/^3/, '4')]),
      );
      renumberHeadings(cppGroup.blocks, cppNumbers);
      cppGroup.title = '4. 从 C 到 C++：';
    }

    const imageGroup = groups.find((group) => group.title.startsWith('5. OpenCV 图像基础'));
    if (imageGroup) {
      renumberHeadings(imageGroup.blocks, {
        '5.0': '7.0', '5.0.1': '7.0.1', '5.1': '7.1', '5.2': '7.2', '5.3': '7.3',
      });
      imageGroup.title = imageGroup.title.replace(/^5\./, '7.');
    }

    const videoGroup = groups.find((group) => group.title.startsWith('6. OpenCV 视频基础'));
    if (videoGroup) {
      renumberHeadings(videoGroup.blocks, { '6.1': '8.1', '6.2': '8.2' });
      videoGroup.title = videoGroup.title.replace(/^6\./, '8.');
    }

    const homeworkGroup = groups.find((group) => group.title.startsWith('7. 第一周综合练习'));
    if (homeworkGroup) homeworkGroup.title = homeworkGroup.title.replace(/^7\./, '9.');

    const ubuntuPosition = cppIndex < 0 ? ubuntuIndex : cppIndex;

    groups.splice(ubuntuPosition, 0, ubuntuLesson);
    groups.splice(ubuntuIndex + (ubuntuPosition <= ubuntuIndex ? 1 : 0), 0, cmakeLesson);
  }
}

const algorithmShowcase = `<section class="course-showcase"><h2>算法组在做什么</h2><p>算法程序从相机画面中找出机器人装甲板，并给出目标位置。下图展示一条入门检测流程：先从原图提取可能发光的区域，再寻找灯条候选，最后把符合几何关系的一对灯条组成装甲板候选，并标出目标中心。</p>
<figure class="lesson-figure showcase-main"><img src="assets/armor-pairing.png" alt="教学示例中的装甲板检测步骤：二值掩膜、灯条候选、几何配对和选中目标框"><figcaption>教学示例：从二值图和候选灯条到装甲板配对。框选结果用于说明检测过程，不代表已完成实车部署。</figcaption></figure>
<div class="showcase-notes"><h3>第一周学到的基础</h3><p>配置 C++ 与 OpenCV 工程，读入并显示图片、视频。</p><h3>第二周学习的内容</h3><p>在给定素材上观察目标掩膜、轮廓、灯条候选和配对框，并输出“是否找到目标”和目标中心点。</p><h3>战队完整系统的进阶方向</h3><p>实际自瞄还要进一步完成空间定位、连续帧跟踪、运动预测、云台通信和整车联调。这些是后续系统工作，不是本周培训承诺完成的内容。</p></div></section>`;

const lessons = groups.map((group, index) => {
  const html = execFileSync('pandoc', ['--from=json', '--to=html5', '--wrap=none'], {
    input: JSON.stringify({
      ...document,
      blocks: group.blocks.filter((block) => !(block.t === 'Header' && block.c[0] === 1)),
    }),
    encoding: 'utf8',
    maxBuffer: 8 * 1024 * 1024,
  });
  const body = html.trim();
  return {
    id: index === 0 ? 'intro' : `source-${index}`,
    title: group.title,
    body: group.title.startsWith('7. OpenCV 图像基础')
      ? `<p><strong>OpenCV</strong> 是一个开源的计算机视觉与图像处理工具库，不是一种编程语言，也不是某个单独的识别算法。它提供了读取和显示图片、处理像素、分析视频等现成能力，让我们可以把精力放在理解和解决视觉问题上。用 C++ 时，OpenCV 通过一组库函数提供这些功能；例如，<code>cv::Mat</code> 用来保存图像数据。可以先把它理解为视觉程序操作图片时常用的数据对象，后面本章会逐步介绍它。</p>${body}`
      : group.title.startsWith('1. 总体路线')
        ? body
        .replace(/第 ([56]) 章/g, (match) => match === '第 5 章' ? '第 7 章' : '第 8 章')
        .replace('下面是一条便于入门的示例路线，实际练习不限定必须使用其中的某一种分割方法：', '先了解算法组希望实现的效果，再沿着下面的路线逐步学习。')
        .replace('<pre class="text">', `${algorithmShowcase}<h2>二维视觉的入门路线</h2><p>从画面到目标中心点，可以拆成下面几个步骤。实际练习不限定必须使用其中的某一种分割方法。</p><pre class="text">`)
        .replace('<p>第一周结束时，读者应能：</p>', '<h2>第一周目标与学习安排</h2><p>第一周结束时，读者应能：</p>')
        : body,
  };
});

const cmakeLessonIndex = lessons.findIndex((lesson) => lesson.title.startsWith('5. CMake'));
lessons.splice(cmakeLessonIndex + 1, 0, {
  id: 'color-spaces',
  chapterTitle: '专项培训_1',
  title: '6. 图像的基础知识：BGR、HSV、LAB、灰度图',
  body: `<p>同一张照片，既可以记录为彩色图，也可以记录为灰度图。<strong>色彩空间</strong>就是描述颜色的一套方式：像给同一个地点换一张不同的“坐标表”，场景本身并没有变。先认识这些表示方式，再学习如何处理图像。</p>
<figure class="lesson-figure"><div class="color-example"><div><img src="assets/color-spaces.png" alt="彩色的林间石板路，周围有绿色、黄色和红色的树叶"><span>彩色原图</span></div><div><img class="grayscale-example" src="assets/color-spaces.png" alt="同一条林间石板路的灰度效果，树叶失去颜色但石路的明暗仍可见"><span>灰度效果</span></div></div><figcaption>同一张林间石路照片：左边保留颜色，右边仅用于直观比较明暗。BGR、HSV 和 Lab 是描述左图颜色的不同方式，正确还原后并不会变成另一幅场景。</figcaption></figure>
<h2>1. 图片由像素组成</h2><p>把照片放大，可以想象它由一格格<strong>像素</strong>组成。每个像素都有一组数字，描述这一小格的颜色或明暗；每一类数字叫一个<strong>通道</strong>。例如彩色照片通常用三个通道记录颜色，灰度图通常只用一个值记录明暗。通道不是照片里的三个物体，而是同一位置的三项信息。</p>
<h2>2. BGR：用蓝、绿、红记录颜色</h2><p><strong>B、G、R</strong> 分别代表蓝、绿、红。三种颜色按不同强度混合，就能表示照片里的一片绿叶、一块灰石板或一簇红叶。对常见的 8 位彩色图，每项数值通常在 0 到 255 之间，0 表示这一项没有贡献，255 表示贡献最大。</p><p>BGR 与更常听到的 RGB 记录的是相同的三种颜色，只是<strong>排列顺序不同</strong>。在 OpenCV 中常见的是 BGR；若把顺序误认为 RGB，红色和蓝色就可能对调。整幅照片并不是只剩蓝、绿、红三种颜色，它们可以组合出丰富的颜色。</p>
<h2>3. HSV：用颜色种类、鲜艳程度和明亮程度描述</h2><p>HSV 换了一种提问方式：这是什么颜色？有多鲜艳？有多亮？</p><ul><li><strong>H（色相）</strong>：大致属于红、黄、绿等哪一类颜色。图中的红叶和绿叶，主要区别就在颜色种类。</li><li><strong>S（饱和度）</strong>：颜色有多鲜明。鲜艳的红叶饱和度较高，接近灰色的石板饱和度较低。</li><li><strong>V（明度）</strong>：颜色有多亮。阳光照到的石路通常比阴影里的石路更亮。</li></ul><p>HSV 适合思考“找出某类颜色”的问题，例如在照片里找偏红的叶子。但光线变化也会影响观察结果，不能只凭一个颜色名称保证识别准确。</p>
<h2>4. Lab：把明暗和颜色方向分开</h2><p>Lab 也用三个量描述颜色，不过侧重点不同：<strong>L*</strong> 表示明亮程度；<strong>a*</strong> 描述从绿到红的颜色方向；<strong>b*</strong> 描述从蓝到黄的颜色方向。这里的 a*、b* 是方向，不是把红色或黄色单独提取出来的一张图片。</p><p>对照照片，可以用 L* 理解明亮的石路与阴影的差别，用 a* 理解绿叶和红叶的差别，用 b* 理解偏蓝和偏黄的差别。Lab 常用于讨论颜色差异，尤其是在希望把明暗变化与颜色变化分开思考时。<strong>它不是一种视觉特效</strong>：用 Lab 描述同一张照片，正常显示时仍然是这片树林。</p>
<h2>5. 灰度图：只看明暗，不看颜色</h2><p>灰度图中每个像素只保留一个明暗值，因此图中的树叶不再呈现红、黄、绿，但石路的亮面、阴影和轮廓通常还能看清。彩色图转成灰度时，会考虑人眼对不同颜色亮度的感受，并不是简单地把三种颜色平均一下。</p><p>灰度图适合先观察轮廓、纹理和亮暗变化；代价是<strong>颜色信息丢失</strong>。两片叶子即使一红一绿，只要明暗接近，在灰度图中就可能难以区分。</p>
<h2>6. 四种表示怎么选</h2><div class="table-wrap"><table><thead><tr><th>表示</th><th>主要描述什么</th><th>看这张照片时可以想到</th></tr></thead><tbody><tr><td>BGR</td><td>蓝、绿、红的混合强度</td><td>保留石路和树叶的完整颜色</td></tr><tr><td>HSV</td><td>颜色种类、鲜艳程度、明亮程度</td><td>区分红叶与绿叶，同时留意光照</td></tr><tr><td>Lab</td><td>明暗、绿红方向、蓝黄方向</td><td>分开思考阴影与叶片颜色的变化</td></tr><tr><td>灰度图</td><td>明暗</td><td>观察石路轮廓和林间阴影</td></tr></tbody></table></div><p><strong>要记住：</strong>BGR、HSV 和 Lab 可以描述同一幅彩色画面，差别在于记录颜色的方式；灰度图则舍弃颜色，只留下明暗。不存在一种方式能在所有任务里都最好。</p>
<h2>想一想</h2><ol><li>图中哪两种颜色的树叶，适合用 HSV 的“色相”来区分？</li><li>为什么灰度图还能看见石路，却不容易分清红叶和绿叶？</li><li>如果阴影让同一种叶子一边亮、一边暗，Lab 的哪一项最直接描述这种变化？</li></ol>`,
});

fs.writeFileSync(outputPath, `window.COURSE = ${JSON.stringify([{ title: '专项培训_1', lessons }])};\n`, 'utf8');
console.log(`Generated ${lessons.length} lessons from ${sourcePath}`);
