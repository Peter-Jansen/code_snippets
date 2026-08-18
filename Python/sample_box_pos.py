def sample_box_pos():
    viewer = nuke.activeViewer()
    v = viewer.node()
    bbox = v['colour_sample_bbox']
    n = v.input(0)
    w = n.width()
    h = n.height()
    pa = n.pixelAspect()
    ratio = (float(w) * pa) / float(h)
    bboxX = bbox.x()
    bboxY = bbox.y() * ratio
    wd = w / 2.0
    hd = h / 2.0
    x = int(round(bboxX * wd + wd))
    y = int(round(bboxY * hd + hd))
    return (x, y)
