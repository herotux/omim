package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.util.UiUtils;

import java.util.List;

/// SingleChoise list view don't add radio button to custom view.
public class IconsAdapter extends ArrayAdapter<Icon>
{
  private int mCheckedPosition = 0;

  public IconsAdapter(Context context, List<Icon> list)
  {
    super(context, 0, 0, list);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final Resources res = getContext().getResources();
    if (convertView == null)
    {
      LayoutInflater inflater = LayoutInflater.from(getContext());
      convertView = inflater.inflate(R.layout.color_row, parent, false);
      convertView.setTag(new SpinnerViewHolder((ImageView) convertView.findViewById(R.id.row_color_image),
          (ImageView) convertView.findViewById(R.id.selected_mark)));
    }
    SpinnerViewHolder holder = (SpinnerViewHolder) convertView.getTag();
    if (position == mCheckedPosition)
    {
      UiUtils.show(holder.tick);
      holder.tick.setImageDrawable(
          UiUtils.drawCircle(0xCCFFFFFF, (int) res.getDimension(R.dimen.dp_x_8), res));
    }
    else
      UiUtils.hide(holder.tick);

    holder.icon.setImageDrawable(
        UiUtils.drawCircleForPin(getItem(position).getType(), (int) res.getDimension(R.dimen.dp_x_16), res));

    return convertView;
  }

  private class SpinnerViewHolder
  {
    ImageView icon;
    ImageView tick;

    public SpinnerViewHolder(ImageView icon, ImageView tick)
    {
      this.icon = icon;
      this.tick = tick;
    }
  }

  public void chooseItem(int position)
  {
    mCheckedPosition = position;
    notifyDataSetChanged();
  }

  public int getCheckedItemPosition()
  {
    return mCheckedPosition;
  }
}
