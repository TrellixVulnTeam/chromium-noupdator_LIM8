// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const { assert } = chai;

// FIXME: Convert to pure functions as these utilities have side effects.
import '../../../front_end/platform/utilities.js';

declare global {
  interface Array<T> {
    remove(value: T, firstOnly: boolean): boolean;
    mergeOrdered(array: T[], comparator: (a: T, b: T) => number): T[];
    intersectOrdered(array: T[], comparator: (a: T, b: T) => number): T[];
    upperBound(value: T, comparator?: (a: T, b: T) => number): number;
    lowerBound(value: T, comparator?: (a: T, b: T) => number): number;
    binaryIndexOf(value: T, comparator: (a: T, b: T) => number): number;
    sortRange(comparator: (a: T, b: T) => number, leftBound: number, rightBound: number, sortWindowLeft: number, sortWindowRight: number): T[];
  }

  interface String {
    trimURL(base: string): string;
    toBase64(): string;
    trimMiddle(maxLength: number): string;
    repeat(length: number): string;
  }

  interface StringConstructor {
    hashCode(value: string): number;
    naturalOrderComparator(a: string, b: string): number;
  }
}

describe('Utilities', () => {
  it('removes values', () => {
    const testArrays = [
      [], [], [], [1], [1], [1], [1, 2, 3, 4, 5, 4, 3, 2, 1], [1, 3, 4, 5, 4, 3, 2, 1], [1, 3, 4, 5, 4, 3, 1],
      [2, 2, 2, 2, 2], [2, 2, 2, 2], [], [2, 2, 2, 1, 2, 2, 3, 2], [2, 2, 1, 2, 2, 3, 2], [1, 3]
    ];

    for (let i = 0; i < testArrays.length; i += 3) {
      let actual = testArrays[i].slice(0);
      let expected = testArrays[i + 1];
      actual.remove(2, true);
      assert.deepStrictEqual(expected, actual, 'Removing firstOnly (true) failed');

      actual = testArrays[i].slice(0);
      expected = testArrays[i + 2];
      actual.remove(2, false);
      assert.deepStrictEqual(expected, actual, 'Removing firstOnly (false) failed');
    }
  });

  it('orders merge intersect', () => {
    function comparator(a: number, b: number) {
      return a - b;
    }

    function count(a: number[], x: number) {
      return a.upperBound(x) - a.lowerBound(x);
    }

    function testAll(a: number[], b: number[]) {
      testOperation(a, b, a.mergeOrdered(b, comparator), Math.max, 'U');
      testOperation(a, b, a.intersectOrdered(b, comparator), Math.min, 'x');
    }

    function testOperation(a: number[], b: number[], actual: number[], checkOperation: (...values: number[]) => number, opName: string) {
      const allValues = a.concat(b).concat(actual);
      let expectedCount: number;
      let actualCount: number;

      for (let i = 0; i < allValues.length; ++i) {
        let value = allValues[i];
        expectedCount = checkOperation(count(a, value), count(b, value));
        actualCount = count(actual, value);
        assert.equal(expectedCount, actualCount,
            'Incorrect result for value: ' + value + ' at [' + a + '] ' + opName + ' [' + b + '] -> [' + actual +
                ']');
      }

      // FIXME: This appears to be a no-op given the fact that sort works in-place.
      assert.deepStrictEqual(actual.sort(), actual, 'Result array is ordered');
    }

    const testArrays = [
      [], [], [1], [], [1, 2, 2, 2, 3], [], [4, 5, 5, 8, 8], [1, 1, 1, 2, 6], [1, 2, 2, 2, 2, 3, 3, 4],
      [2, 2, 2, 3, 3, 3, 3], [1, 2, 3, 4, 5], [1, 2, 3]
    ];

    for (let i = 0; i < testArrays.length; i += 2) {
      testAll(testArrays[i], testArrays[i + 1]);
      testAll(testArrays[i + 1], testArrays[i]);
    }
  });

  it('calculates the binary index', () => {
    const testArrays = [
      [], [1], [1, 10], [1, 10, 11, 12, 13, 14, 100], [-100, -50, 0, 50, 100], [-100, -14, -13, -12, -11, -10, -1]
    ];

    function testArray(array: number[]) {
      function comparator(a: number, b: number) {
        return a < b ? -1 : (a > b ? 1 : 0);
      }

      for (let i = -100; i <= 100; ++i) {
        let reference = array.indexOf(i);
        let actual = array.binaryIndexOf(i, comparator);
        assert.deepStrictEqual(reference, actual);
      }
      return true;
    }

    for (let i = 0, l = testArrays.length; i < l; ++i) {
      testArray(testArrays[i]);
    }
  });

  it('calculates the lower bound', () => {
    const testArrays = [[], [1], [-1, -1, 0, 0, 0, 0, 2, 3, 4, 4, 4, 7, 9, 9, 9]];

    function testArray(array: number[], useComparator: boolean) {
      function comparator(a: number, b: number) {
        return a < b ? -1 : (a > b ? 1 : 0);
      }

      for (let value = -2; value <= 12; ++value) {
        let index = useComparator ? array.lowerBound(value, comparator) : array.lowerBound(value);
        assert.isTrue(0 <= index && index <= array.length, 'index is not within bounds');
        assert.isTrue(index === 0 || array[index - 1] < value, 'array[index - 1] >= value');
        assert.isTrue(index === array.length || array[index] >= value, 'array[index] < value');
      }
    }

    for (let i = 0, l = testArrays.length; i < l; ++i) {
      testArray(testArrays[i], false);
      testArray(testArrays[i], true);
    }
  });

  it('calculates the upper bound', () => {
    const testArrays = [[], [1], [-1, -1, 0, 0, 0, 0, 2, 3, 4, 4, 4, 7, 9, 9, 9]];

    function testArray(array: number[], useComparator: boolean) {
      function comparator(a: number, b: number) {
        return a < b ? -1 : (a > b ? 1 : 0);
      }

      for (let value = -2; value <= 12; ++value) {
        let index = useComparator ? array.upperBound(value, comparator) : array.upperBound(value);
        assert.isTrue(0 <= index && index <= array.length, 'index is out of bounds');
        assert.isTrue(index === 0 || array[index - 1] <= value, 'array[index - 1] > value');
        assert.isTrue(index === array.length || array[index] > value, 'array[index] <= value');
      }
    }

    for (let i = 0, l = testArrays.length; i < l; ++i) {
      testArray(testArrays[i], false);
      testArray(testArrays[i], true);
    }
  });

  it('sorts ranges', () => {
    const testArrays = [[], [1], [2, 1], [6, 4, 2, 7, 10, 15, 1], [10, 44, 3, 6, 56, 66, 10, 55, 32, 56, 2, 5]];

    function testArray(array: number[]) {
      function comparator(a: number, b: number) {
        return a < b ? -1 : (a > b ? 1 : 0);
      }

      for (let left = 0, l = array.length - 1; left < l; ++left) {
        for (let right = left, r = array.length; right < r; ++right) {
          for (let first = left; first <= right; ++first) {
            for (let count = 1, k = right - first + 1; count <= k; ++count) {
              let actual = array.slice(0);
              actual.sortRange(comparator, left, right, first, first + count - 1);
              assert.deepStrictEqual(array.slice(0, left), actual.slice(0, left), 'left ' + left + ' ' + right + ' ' + count);
              assert.deepStrictEqual(
                  array.slice(right + 1), actual.slice(right + 1), 'right ' + left + ' ' + right + ' ' + count);

              let middle = array.slice(left, right + 1);
              middle.sort(comparator);
              assert.deepStrictEqual(
                  middle.slice(first - left, first - left + count), actual.slice(first, first + count),
                  'sorted ' + left + ' ' + right + ' ' + first + ' ' + count);

              const actualRest = actual.slice(first + count, right + 1);
              actualRest.sort(comparator);
              assert.deepStrictEqual(
                  middle.slice(first - left + count), actualRest,
                  'unsorted ' + left + ' ' + right + ' ' + first + ' ' + count);
            }
          }
        }
      }
    }

    for (let i = 0, len = testArrays.length; i < len; ++i) {
      testArray(testArrays[i]);
    }
  });

  it('sorts natural order', () => {
    const testArray = [
      'dup', 'a1',   'a4222',  'a91',       'a07',      'dup', 'a7',        'a007',      'abc00',     'abc0',
      'abc', 'abcd', 'abc000', 'x10y20z30', 'x9y19z29', 'dup', 'x09y19z29', 'x10y22z23', 'x10y19z43', '1',
      '10',  '11',   'dup',    '2',         '2',        '2',   '555555',    '5',         '5555',      'dup',
    ];

    for (let i = 0, n = testArray.length; i < n; ++i) {
      assert.equal(0, String.naturalOrderComparator(testArray[i], testArray[i]), 'comparing equal strings');
    }

    testArray.sort(String.naturalOrderComparator);

    // Check comparator's transitivity.
    for (let i = 0, n = testArray.length; i < n; ++i) {
      for (let j = 0; j < n; ++j) {
        const a = testArray[i];
        const b = testArray[j];
        const diff = String.naturalOrderComparator(a, b);
        if (diff === 0)
          assert.equal(a, b, 'zero diff');
        else if (diff < 0)
          assert.isTrue(i < j);
        else
          assert.isTrue(i > j);
      }
    }
  });

  it('hashes strings', () => {
    const stringA = ' '.repeat(10000);
    const stringB = stringA + ' ';
    const hashA = String.hashCode(stringA);
    assert.isTrue(hashA !== String.hashCode(stringB));
    assert.isTrue(isFinite(hashA));
    assert.isTrue(hashA + 1 !== hashA);
  });

  it('trims URLs', () => {
    const baseURLDomain = 'www.chromium.org';
    const testArray = [
      'http://www.chromium.org/foo/bar',
      '/foo/bar',
      'https://www.CHromium.ORG/BAZ/zoo',
      '/BAZ/zoo',
      'https://example.com/foo[]',
      'example.com/foo[]',
    ];
    for (let i = 0; i < testArray.length; i += 2) {
      const url = testArray[i];
      const expected = testArray[i + 1];
      assert.equal(expected, url.trimURL(baseURLDomain), url);
    }
  });

  it('converts to base64', () => {
    const testArray = [
      '', '', 'a', 'YQ==', 'bc', 'YmM=', 'def', 'ZGVm', 'ghij', 'Z2hpag==', 'klmno', 'a2xtbm8=', 'pqrstu', 'cHFyc3R1',
      String.fromCharCode(0x444, 0x5555, 0x66666, 0x777777), '0YTllZXmmabnnbc='
    ];
    for (let i = 0; i < testArray.length; i += 2) {
      const string = testArray[i];
      const encodedString = testArray[i + 1];
      assert.equal(encodedString, string.toBase64());
    }
  });

  it('trims the middle of strings', () => {
    const testArray = [
      '', '!', '\uD83D\uDE48A\uD83D\uDE48L\uD83D\uDE48I\uD83D\uDE48N\uD83D\uDE48A\uD83D\uDE48\uD83D\uDE48', 'test'
    ];
    for (let string of testArray) {
      for (let maxLength = string.length + 1; maxLength > 0; --maxLength) {
        const trimmed = string.trimMiddle(maxLength);
        assert.isTrue(trimmed.length <= maxLength);
      }
    }
  });
});
