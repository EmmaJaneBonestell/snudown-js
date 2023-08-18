mergeInto(LibraryManager.library, {
  _cryptoGetRandomValues: function(buffer, size) {
    var array = new Uint8Array(size);
	if (typeof crypto !== 'undefined' && crypto.getRandomValues) {
      // For browsers
      crypto.getRandomValues(array);
    } else if (typeof window !== 'undefined' && window.crypto && window.crypto.getRandomValues) {
      // For older browsers where crypto might be on the window object
      window.crypto.getRandomValues(array);
    } else if (typeof crypto !== 'undefined' && crypto.randomFillSync) {
      // For Node.js
      crypto.randomFillSync(array);
    } else {
      throw new Error("A suitable random number generator was not found.");
    }
    for (var i = 0; i < size; i++) {
        setValue(buffer + i, array[i], 'i8');
    }
    return size;
  }
});
