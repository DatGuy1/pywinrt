
import asyncio
import unittest

import winrt.windows.foundation.collections as wfc

from ._util import async_test

class TestCollections(unittest.TestCase):

    def test_stringmap(self):
        m = wfc.StringMap()
        m.insert("hello", "world")

        self.assertTrue(m.has_key("hello"))
        self.assertFalse(m.has_key("world"))
        self.assertEqual(m.size, 1)
        self.assertEqual(m.lookup("hello"), "world")

        m.remove("hello")
        self.assertRaises(OSError, lambda: m.lookup("hello"))

    def test_stringmap_is_mapping(self):
        m = wfc.StringMap()
        m["hello"] = "world"

        self.assertEqual(len(m), 1)
        self.assertEqual(m["hello"], "world")
        self.assertIn("hello", m)

        del m["hello"]
        self.assertNotIn("hello", m)

    @async_test
    async def test_stringmap_changed_event(self):
        loop = asyncio.get_running_loop()
        future = loop.create_future()

        def on_map_changed(sender: wfc.IMap, args: wfc.IMapChangedEventArgs): 
            try:
                self.assertEqual(args.collection_change, wfc.CollectionChange.ITEM_INSERTED)
                self.assertEqual(args.key, "dr")

                self.assertEqual(sender.size, 2)
                self.assertTrue(sender.has_key("dr"))
                self.assertTrue(sender.has_key("hello"))
            
                loop.call_soon_threadsafe(future.set_result, True)
            except Exception as ex:
                loop.call_soon_threadsafe(future.set_exception, ex)

        m = wfc.StringMap()
        m.insert("hello", "world")
        token = m.add_map_changed(on_map_changed)
        m.insert("dr", "who")
        m.remove_map_changed(token)

        called = await asyncio.wait_for(future, 1)
        self.assertTrue(called)


if __name__ == '__main__':
    unittest.main()