/*
 * (C) Copyright 2014 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "kmstreebin.h"

#define GST_DEFAULT_NAME "treebin"
#define GST_CAT_DEFAULT kms_tree_bin_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define kms_tree_bin_parent_class parent_class
G_DEFINE_TYPE (KmsTreeBin, kms_tree_bin, GST_TYPE_BIN);

#define KMS_TREE_BIN_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (         \
    (obj),                              \
    KMS_TYPE_TREE_BIN,                  \
    KmsTreeBinPrivate                   \
  )                                     \
)

#define INPUT_QUEUE_NAME "input_queue"
#define OUTPUT_TEE_NAME "output_tee"

struct _KmsTreeBinPrivate
{
  GstElement *input_queue, *output_tee;
};

GstElement *
kms_tree_bin_get_input_queue (KmsTreeBin * self)
{
  return self->priv->input_queue;
}

GstElement *
kms_tree_bin_get_output_tee (KmsTreeBin * self)
{
  return self->priv->output_tee;
}

void
kms_tree_bin_unlink_input_queue_from_tee (KmsTreeBin * self)
{
  GstPad *queue_sink, *peer, *tee_src;

  queue_sink = gst_element_get_static_pad (self->priv->input_queue, "sink");
  peer = gst_pad_get_peer (queue_sink);

  if (GST_IS_PROXY_PAD (peer)) {
    GstProxyPad *ghost;

    ghost = gst_proxy_pad_get_internal (GST_PROXY_PAD (peer));
    tee_src = gst_pad_get_peer (GST_PAD (ghost));

    g_object_unref (peer);
    g_object_unref (ghost);
  } else {
    tee_src = peer;
  }

  gst_pad_unlink (tee_src, queue_sink);
  gst_element_release_request_pad (GST_ELEMENT (GST_OBJECT_PARENT (tee_src)),
      tee_src);

  g_object_unref (tee_src);
  g_object_unref (queue_sink);
}

static void
kms_tree_bin_init (KmsTreeBin * self)
{
  GstElement *fakequeue, *fakesink;

  self->priv = KMS_TREE_BIN_GET_PRIVATE (self);

  self->priv->input_queue =
      gst_element_factory_make ("queue", INPUT_QUEUE_NAME);
  self->priv->output_tee = gst_element_factory_make ("tee", OUTPUT_TEE_NAME);
  fakequeue = gst_element_factory_make ("queue", NULL);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  g_object_set (fakesink, "async", FALSE, NULL);

  gst_bin_add_many (GST_BIN (self), self->priv->input_queue,
      self->priv->output_tee, fakequeue, fakesink, NULL);
  gst_element_link_many (self->priv->output_tee, fakequeue, fakesink, NULL);
}

static void
kms_tree_bin_class_init (KmsTreeBinClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_set_details_simple (gstelement_class,
      "TreeBin",
      "Generic",
      "Base bin to manage elements for media distribution.",
      "Miguel París Díaz <mparisdiaz@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
      GST_DEFAULT_NAME);

  g_type_class_add_private (klass, sizeof (KmsTreeBinPrivate));
}